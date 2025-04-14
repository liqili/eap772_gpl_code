/* vi: set sw=4 ts=4: */
/*
 * Mini syslogd implementation for busybox
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Copyright (C) 2000 by Karl M. Hegbloom <karlheg@debian.org>
 *
 * "circular buffer" Copyright (C) 2001 by Gennady Feldman <gfeldman@gena01.com>
 *
 * Maintainer: Gennady Feldman <gfeldman@gena01.com> as of Mar 12, 2001
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

//usage:#define syslogd_trivial_usage
//usage:       "[OPTIONS]"
//usage:#define syslogd_full_usage "\n\n"
//usage:       "System logging utility\n"
//usage:	IF_NOT_FEATURE_SYSLOGD_CFG(
//usage:       "(this version of syslogd ignores /etc/syslog.conf)\n"
//usage:	)
//usage:     "\n	-n		Run in foreground"
//usage:     "\n	-O FILE		Log to FILE (default:/var/log/messages)"
//usage:     "\n	-l N		Log only messages more urgent than prio N (1-8)"
//usage:     "\n	-S		Smaller output"
//usage:	IF_FEATURE_ROTATE_LOGFILE(
//usage:     "\n	-s SIZE		Max size (KB) before rotation (default:200KB, 0=off)"
//usage:     "\n	-b N		N rotated logs to keep (default:1, max=99, 0=purge)"
//usage:	)
//usage:	IF_FEATURE_REMOTE_LOG(
//usage:     "\n	-R HOST[:PORT]	Log to IP or hostname on PORT (default PORT=514/UDP)"
//usage:     "\n	-L		Log locally and via network (default is network only if -R)"
//usage:	)
//usage:	IF_FEATURE_SYSLOGD_DUP(
//usage:     "\n	-D		Drop duplicates"
//usage:	)
//usage:	IF_FEATURE_IPC_SYSLOG(
/* NB: -Csize shouldn't have space (because size is optional) */
//usage:     "\n	-C[size_kb]	Log to shared mem buffer (use logread to read it)"
//usage:	)
//usage:	IF_FEATURE_SYSLOGD_CFG(
//usage:     "\n	-f FILE		Use FILE as config (default:/etc/syslog.conf)"
//usage:	)
/* //usage:  "\n	-m MIN		Minutes between MARK lines (default:20, 0=off)" */
//usage:
//usage:#define syslogd_example_usage
//usage:       "$ syslogd -R masterlog:514\n"
//usage:       "$ syslogd -R 192.168.1.1:601\n"

/*
 * Done in syslogd_and_logger.c:
#include "libbb.h"
#define SYSLOG_NAMES
#define SYSLOG_NAMES_CONST
#include <syslog.h>
*/

#include <sys/un.h>
#include <sys/uio.h>
#include <sys/sysinfo.h>
#include <errno.h>

#if ENABLE_FEATURE_REMOTE_LOG
#include <netinet/in.h>
#endif

#if ENABLE_FEATURE_IPC_SYSLOG
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#endif


#define DEBUG 0

/* MARK code is not very useful, is bloat, and broken:
 * can deadlock if alarmed to make MARK while writing to IPC buffer
 * (semaphores are down but do_mark routine tries to down them again) */
#undef SYSLOGD_MARK

/* Write locking does not seem to be useful either */
#undef SYSLOGD_WRLOCK

/* unix socket of maglogd and syslogd */
#define TP_PATH_LOG "/var/log"

/* equal to macro and data structure defined in apps/msglogd/libmsglogd.c */
#define MAX_MSGLOG_LEN		(500)
typedef struct _LOG_DATA
{
	int logTimeFlag;
	int dataLen;
	char data[MAX_MSGLOG_LEN];
} LOG_DATA;

/* default MAX_READ is 256, but it changed to 512 for EAP, see busybox.config */
enum {
	MAX_READ = CONFIG_FEATURE_SYSLOGD_READ_BUFFER_SIZE,
	DNS_WAIT_SEC = 2 * 60,
};

/* Semaphore operation structures */
/* 原busybox-1.20.2的代码移除了head指针，但由于我们在读取log时需要head定位首条log
 * 因此此处仍使用busybox-1.01处的定义
 */
struct shbuf_ds {
	int32_t size;   /* size of data - 1 */
	int32_t head;	/* start piont for log read */
	int32_t tail;   /* end of message list */
	char data[1];   /* data/messages */
};

#if ENABLE_FEATURE_REMOTE_LOG
typedef struct {
	int remoteFD;
	unsigned last_dns_resolve;
	len_and_sockaddr *remoteAddr;
	const char *remoteHostname;
} remoteHost_t;
#endif

typedef struct logFile_t {
	const char *path;
	int fd;
#if ENABLE_FEATURE_ROTATE_LOGFILE
	unsigned size;
	uint8_t isRegular;
#endif
} logFile_t;

#if ENABLE_FEATURE_SYSLOGD_CFG
typedef struct logRule_t {
	uint8_t enabled_facility_priomap[LOG_NFACILITIES];
	struct logFile_t *file;
	struct logRule_t *next;
} logRule_t;
#endif

/* Allows us to have smaller initializer. Ugly. */
#define GLOBALS \
	logFile_t logFile;                      \
	/* interval between marks in seconds */ \
	/*int markInterval;*/                   \
	/* level of messages to be logged */    \
	int logLevel;                           \
IF_FEATURE_ROTATE_LOGFILE( \
	/* max size of file before rotation */  \
	unsigned logFileSize;                   \
	/* number of rotated message files */   \
	unsigned logFileRotate;                 \
) \
IF_FEATURE_IPC_SYSLOG( \
	int shmid; /* ipc shared memory id */   \
	int s_semid; /* ipc semaphore id */     \
	int shm_size;                           \
	struct sembuf SMwup[1];                 \
	struct sembuf SMwdn[3];                 \
) \
IF_FEATURE_SYSLOGD_CFG( \
	logRule_t *log_rules; \
) \
IF_FEATURE_REMOTE_LOG(\
	int remotelogLevel; \
) 

struct init_globals {
	GLOBALS
};

struct globals {
	GLOBALS

#if ENABLE_FEATURE_REMOTE_LOG
	llist_t *remoteHosts;
#endif
#if ENABLE_FEATURE_IPC_SYSLOG
	struct shbuf_ds *shbuf;
#endif
	time_t last_log_time;
	/* localhost's name. We print only first 64 chars */
	char *hostname;

	/* We recv into recvbuf... */
	char recvbuf[MAX_READ * (1 + ENABLE_FEATURE_SYSLOGD_DUP)];
	/* ...then copy to parsebuf, escaping control chars */
	/* (can grow x2 max) */
	char parsebuf[MAX_READ*2];
	/* ...then sprintf into printbuf, adding timestamp (15 chars),
	 * host (64), fac.prio (20) to the message */
	/* (growth by: 15 + 64 + 20 + delims = ~110) */
	char printbuf[MAX_READ*2 + 128];
};

static const struct init_globals init_data = {
	.logFile = {
		.path = "/var/log/messages",
		.fd = -1,
	},
#ifdef SYSLOGD_MARK
	.markInterval = 20 * 60,
#endif
	.logLevel = 8,
#if ENABLE_FEATURE_ROTATE_LOGFILE
	.logFileSize = 200 * 1024,
	.logFileRotate = 1,
#endif
#if ENABLE_FEATURE_IPC_SYSLOG
	.shmid = -1,
	.s_semid = -1,
	.shm_size = ((CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE)*1024), /* default shm size */
	.SMwup = { {1, -1, IPC_NOWAIT} },
	.SMwdn = { {0, 0}, {1, 0}, {1, +1} },
#endif

#if ENABLE_FEATURE_REMOTE_LOG
	.remotelogLevel = 8,
#endif
};

#define G (*ptr_to_globals)
#define INIT_G() do { \
	SET_PTR_TO_GLOBALS(memcpy(xzalloc(sizeof(G)), &init_data, sizeof(init_data))); \
} while (0)


/* Options */
enum {
	OPTBIT_mark = 0, // -m
	OPTBIT_nofork, // -n
	OPTBIT_outfile, // -O
	OPTBIT_loglevel, // -l
	OPTBIT_small, // -S
	IF_FEATURE_ROTATE_LOGFILE(OPTBIT_filesize   ,)	// -s
	IF_FEATURE_ROTATE_LOGFILE(OPTBIT_rotatecnt  ,)	// -b
	IF_FEATURE_REMOTE_LOG(    OPTBIT_remotelog  ,)	// -R
	IF_FEATURE_REMOTE_LOG(    OPTBIT_remoteloglevel  ,)	// -r, 为EAP添加，允许限制发送到源端的log级别
	IF_FEATURE_REMOTE_LOG(    OPTBIT_locallog   ,)	// -L
	IF_FEATURE_IPC_SYSLOG(    OPTBIT_circularlog,)	// -C
	IF_FEATURE_SYSLOGD_DUP(   OPTBIT_dup        ,)	// -D
	IF_FEATURE_SYSLOGD_CFG(   OPTBIT_cfg        ,)	// -f

	OPT_mark        = 1 << OPTBIT_mark    ,
	OPT_nofork      = 1 << OPTBIT_nofork  ,
	OPT_outfile     = 1 << OPTBIT_outfile ,
	OPT_loglevel    = 1 << OPTBIT_loglevel,
	OPT_small       = 1 << OPTBIT_small   ,
	OPT_filesize    = IF_FEATURE_ROTATE_LOGFILE((1 << OPTBIT_filesize   )) + 0,
	OPT_rotatecnt   = IF_FEATURE_ROTATE_LOGFILE((1 << OPTBIT_rotatecnt  )) + 0,
	OPT_remotelog   = IF_FEATURE_REMOTE_LOG(    (1 << OPTBIT_remotelog  )) + 0,
	OPT_remoteloglevel   = IF_FEATURE_REMOTE_LOG(    (1 << OPTBIT_remoteloglevel  )) + 0,
	OPT_locallog    = IF_FEATURE_REMOTE_LOG(    (1 << OPTBIT_locallog   )) + 0,
	OPT_circularlog = IF_FEATURE_IPC_SYSLOG(    (1 << OPTBIT_circularlog)) + 0,
	OPT_dup         = IF_FEATURE_SYSLOGD_DUP(   (1 << OPTBIT_dup        )) + 0,
	OPT_cfg         = IF_FEATURE_SYSLOGD_CFG(   (1 << OPTBIT_cfg        )) + 0,
};
#define OPTION_STR "m:nO:l:S" \
	IF_FEATURE_ROTATE_LOGFILE("s:" ) \
	IF_FEATURE_ROTATE_LOGFILE("b:" ) \
	IF_FEATURE_REMOTE_LOG(    "R:" ) \
	IF_FEATURE_REMOTE_LOG(    "r:" ) \
	IF_FEATURE_REMOTE_LOG(    "L"  ) \
	IF_FEATURE_IPC_SYSLOG(    "C::") \
	IF_FEATURE_SYSLOGD_DUP(   "D"  ) \
	IF_FEATURE_SYSLOGD_CFG(   "f:"  )
#define OPTION_DECL *opt_m, *opt_l \
	IF_FEATURE_ROTATE_LOGFILE(,*opt_s) \
	IF_FEATURE_ROTATE_LOGFILE(,*opt_b) \
	IF_FEATURE_REMOTE_LOG(	  ,*opt_r) \
	IF_FEATURE_IPC_SYSLOG(    ,*opt_C = NULL) \
	IF_FEATURE_SYSLOGD_CFG(   ,*opt_f = NULL)
#define OPTION_PARAM &opt_m, &(G.logFile.path), &opt_l \
	IF_FEATURE_ROTATE_LOGFILE(,&opt_s) \
	IF_FEATURE_ROTATE_LOGFILE(,&opt_b) \
	IF_FEATURE_REMOTE_LOG(	  ,&remoteAddrList) \
	IF_FEATURE_REMOTE_LOG(	  ,&opt_r) \
	IF_FEATURE_IPC_SYSLOG(    ,&opt_C) \
	IF_FEATURE_SYSLOGD_CFG(   ,&opt_f)

/* New log need to check time, by Sun Jincheng 20Jan17 */
#define LOG_TIME_NOT_CORRECT ('0')
#define LOG_TIME_IS_CORRECT ('1')
#define PID_FILE_LEN		30

static char logTimeFlag = LOG_TIME_NOT_CORRECT;

const char l_syslogPidFile[PID_FILE_LEN] = "/var/run/syslogd.pid";

#if ENABLE_FEATURE_SYSLOGD_CFG
static const CODE* find_by_name(char *name, const CODE* c_set)
{
	for (; c_set->c_name; c_set++) {
		if (strcmp(name, c_set->c_name) == 0)
			return c_set;
	}
	return NULL;
}
#endif
static const CODE* find_by_val(int val, const CODE* c_set)
{
	for (; c_set->c_name; c_set++) {
		if (c_set->c_val == val)
			return c_set;
	}
	return NULL;
}
static int try_to_resolve_remote(remoteHost_t *rh);

#if ENABLE_FEATURE_SYSLOGD_CFG
static void parse_syslogdcfg(const char *file)
{
	char *t;
	logRule_t **pp_rule;
	/* tok[0] set of selectors */
	/* tok[1] file name */
	/* tok[2] has to be NULL */
	char *tok[3];
	parser_t *parser;

	parser = config_open2(file ? file : "/etc/syslog.conf",
				file ? xfopen_for_read : fopen_for_read);
	if (!parser)
		/* didn't find default /etc/syslog.conf */
		/* proceed as if we built busybox without config support */
		return;

	/* use ptr to ptr to avoid checking whether head was initialized */
	pp_rule = &G.log_rules;
	/* iterate through lines of config, skipping comments */
	while (config_read(parser, tok, 3, 2, "# \t", PARSE_NORMAL | PARSE_MIN_DIE)) {
		char *cur_selector;
		logRule_t *cur_rule;

		/* unexpected trailing token? */
		if (tok[2])
			goto cfgerr;

		cur_rule = *pp_rule = xzalloc(sizeof(*cur_rule));

		cur_selector = tok[0];
		/* iterate through selectors: "kern.info;kern.!err;..." */
		do {
			const CODE *code;
			char *next_selector;
			uint8_t negated_prio; /* "kern.!err" */
			uint8_t single_prio;  /* "kern.=err" */
			uint32_t facmap; /* bitmap of enabled facilities */
			uint8_t primap;  /* bitmap of enabled priorities */
			unsigned i;

			next_selector = strchr(cur_selector, ';');
			if (next_selector)
				*next_selector++ = '\0';

			t = strchr(cur_selector, '.');
			if (!t)
				goto cfgerr;
			*t++ = '\0'; /* separate facility from priority */

			negated_prio = 0;
			single_prio = 0;
			if (*t == '!') {
				negated_prio = 1;
				++t;
			}
			if (*t == '=') {
				single_prio = 1;
				++t;
			}

			/* parse priority */
			if (*t == '*')
				primap = 0xff; /* all 8 log levels enabled */
			else {
				uint8_t priority;
				code = find_by_name(t, prioritynames);
				if (!code)
					goto cfgerr;
				primap = 0;
				priority = code->c_val;
				if (priority == INTERNAL_NOPRI) {
					/* ensure we take "enabled_facility_priomap[fac] &= 0" branch below */
					negated_prio = 1;
				} else {
					priority = 1 << priority;
					do {
						primap |= priority;
						if (single_prio)
							break;
						priority >>= 1;
					} while (priority);
					if (negated_prio)
						primap = ~primap;
				}
			}

			/* parse facility */
			if (*cur_selector == '*')
				facmap = (1<<LOG_NFACILITIES) - 1;
			else {
				char *next_facility;
				facmap = 0;
				t = cur_selector;
				/* iterate through facilities: "kern,daemon.<priospec>" */
				do {
					next_facility = strchr(t, ',');
					if (next_facility)
						*next_facility++ = '\0';
					code = find_by_name(t, facilitynames);
					if (!code)
						goto cfgerr;
					/* "mark" is not a real facility, skip it */
					if (code->c_val != INTERNAL_MARK)
						facmap |= 1<<(LOG_FAC(code->c_val));
					t = next_facility;
				} while (t);
			}

			/* merge result with previous selectors */
			for (i = 0; i < LOG_NFACILITIES; ++i) {
				if (!(facmap & (1<<i)))
					continue;
				if (negated_prio)
					cur_rule->enabled_facility_priomap[i] &= primap;
				else
					cur_rule->enabled_facility_priomap[i] |= primap;
			}

			cur_selector = next_selector;
		} while (cur_selector);

		/* check whether current file name was mentioned in previous rules or
		 * as global logfile (G.logFile).
		 */
		if (strcmp(G.logFile.path, tok[1]) == 0) {
			cur_rule->file = &G.logFile;
			goto found;
		}
		/* temporarily use cur_rule as iterator, but *pp_rule still points
		 * to currently processing rule entry.
		 * NOTE: *pp_rule points to the current (and last in the list) rule.
		 */
		for (cur_rule = G.log_rules; cur_rule != *pp_rule; cur_rule = cur_rule->next) {
			if (strcmp(cur_rule->file->path, tok[1]) == 0) {
				/* found - reuse the same file structure */
				(*pp_rule)->file = cur_rule->file;
				cur_rule = *pp_rule;
				goto found;
			}
		}
		cur_rule->file = xzalloc(sizeof(*cur_rule->file));
		cur_rule->file->fd = -1;
		cur_rule->file->path = xstrdup(tok[1]);
 found:
		pp_rule = &cur_rule->next;
	}
	config_close(parser);
	return;

 cfgerr:
	bb_error_msg_and_die("error in '%s' at line %d", file, parser->lineno);
}
#endif

/* circular buffer variables/structures */
#if ENABLE_FEATURE_IPC_SYSLOG

#if CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE < 4
#error Sorry, you must set the syslogd buffer size to at least 4KB.
#error Please check CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE
#endif

/* our shared key (syslogd.c and logread.c must be in sync) */
enum { KEY_ID = 0x414e4547 }; /* "GENA" */

static void timestamp_and_log(int pri, char *msg, int len);

#if 0
static void ipcsyslog_cleanup(void)
{
	printf("clean shm\n");
	unlink (l_syslogPidFile);
	if (G.shmid != -1) {
		shmdt(G.shbuf);
	}
	if (G.shmid != -1) {
		shmctl(G.shmid, IPC_RMID, NULL);
	}
	if (G.s_semid != -1) {
		semctl(G.s_semid, 0, IPC_RMID, 0);
	}
}
#endif

static void ipcsyslog_init(void)
{
	int shm_exist = 0;
	if (DEBUG)
		printf("shmget(%x, %d,...)\n", (int)KEY_ID, G.shm_size);

	/*G.shmid = shmget(KEY_ID, G.shm_size, IPC_CREAT | 0644);
	if (G.shmid == -1) {
		bb_perror_msg_and_die("shmget");
	}*/
	if ((G.shmid = shmget(KEY_ID, G.shm_size, IPC_CREAT | 1023 | IPC_EXCL)) == -1)
	{
		if (errno == EEXIST)
		{
			shm_exist = 1;
			printf("no need to recreate shm\n");
			if ((G.shmid = shmget(KEY_ID, G.shm_size, IPC_CREAT | 1023)) == -1)
			{
				bb_perror_msg_and_die("semget");
			}
		}
		else
		{
			bb_perror_msg_and_die("shmget");
		}
	}

	G.shbuf = shmat(G.shmid, NULL, 0);
	if (G.shbuf == (void*) -1L) { /* shmat has bizarre error return */
		bb_perror_msg_and_die("shmat");
	}

	/*memset(G.shbuf, 0, G.shm_size);
	G.shbuf->size = G.shm_size - offsetof(struct shbuf_ds, data) - 1;*/
	/*G.shbuf->tail = 0;*/
	if (0 == shm_exist)
	{
		printf("first create the shared memory, reset it\n");
		memset(G.shbuf, 0, G.shm_size);
		G.shbuf->size = G.shm_size - offsetof(struct shbuf_ds, data) - 1;
	}
	else
	{
		printf("not first create, do not reset it\n");
	}

	/* we'll trust the OS to set initial semval to 0 (let's hope) */
	G.s_semid = semget(KEY_ID, 2, IPC_CREAT | IPC_EXCL | 1023);
	if (G.s_semid == -1) {
		if (errno == EEXIST) {
			G.s_semid = semget(KEY_ID, 2, 0);
			if (G.s_semid != -1)
				return;
		}
		bb_perror_msg_and_die("semget");
	}

	if (shm_exist == 0)
	{
		/* first started, syslog is initing, write the log
		 * This message has type:'OTHER', level:'INFO', content:'System started'
		 * it is not suitable, but can ensure that the first log is 'System started'
		 */
		timestamp_and_log(6, (char *)"<;6>System started", strlen("<;6>System started"));
	}
	
}

void circ_message(const char *msg);

/* 
 * 同样的，由于保留了共享内存结构体的head指针，此处循环写log的函数也仍拷贝
 * busybox-1.01中的函数
 */
void circ_message(const char *msg)
{
	int l = strlen(msg) + 1;	/* count the whole message w/ '\0' included */

	if (semop(G.s_semid, G.SMwdn, 3) == -1) {
		bb_perror_msg_and_die("semop[SMwdn]");
	}

	/*
	 * Circular Buffer Algorithm:
	 * --------------------------
	 *
	 * Start-off w/ empty buffer of specific size SHM_SIZ
	 * Start filling it up w/ messages. I use '\0' as separator to break up messages.
	 * This is also very handy since we can do printf on message.
	 *
	 * Once the buffer is full we need to get rid of the first message in buffer and
	 * insert the new message. (Note: if the message being added is >1 message then
	 * we will need to "remove" >1 old message from the buffer). The way this is done
	 * is the following:
	 *      When we reach the end of the buffer we set a mark and start from the beginning.
	 *      Now what about the beginning and end of the buffer? Well we have the "head"
	 *      index/pointer which is the starting point for the messages and we have "tail"
	 *      index/pointer which is the ending point for the messages. When we "display" the
	 *      messages we start from the beginning and continue until we reach "tail". If we
	 *      reach end of buffer, then we just start from the beginning (offset 0). "head" and
	 *      "tail" are actually offsets from the beginning of the buffer.
	 *
	 * Note: This algorithm uses Linux IPC mechanism w/ shared memory and semaphores to provide
	 *       a threasafe way of handling shared memory operations.
	 */
	if ((G.shbuf->tail + l) < G.shbuf->size) {
		/* before we append the message we need to check the HEAD so that we won't
		   overwrite any of the message that we still need and adjust HEAD to point
		   to the next message! */
		if (G.shbuf->tail < G.shbuf->head) {
			if ((G.shbuf->tail + l) >= G.shbuf->head) {
				/* we need to move the HEAD to point to the next message
				 * Theoretically we have enough room to add the whole message to the
				 * buffer, because of the first outer IF statement, so we don't have
				 * to worry about overflows here!
				 */
				int k = G.shbuf->tail + l - G.shbuf->head;	/* we need to know how many bytes
													   we are overwriting to make
													   enough room */
				char *c =
					memchr(G.shbuf->data + G.shbuf->head + k, '\0',
						   G.shbuf->size - (G.shbuf->head + k));
				if (c != NULL) {	/* do a sanity check just in case! */
					G.shbuf->head = c - G.shbuf->data + 1;	/* we need to convert pointer to
													   offset + skip the '\0' since
													   we need to point to the beginning
													   of the next message */
					/* Note: HEAD is only used to "retrieve" messages, it's not used
					   when writing messages into our buffer */
				} else {	/* show an error message to know we messed up? */
					printf("Weird! Can't find the terminator token??? \n");
					G.shbuf->head = 0;
				}
			}
		}

		/* in other cases no overflows have been done yet, so we don't care! */
		/* we should be ok to append the message now */
		strncpy(G.shbuf->data + G.shbuf->tail, msg, l);	/* append our message */
		G.shbuf->tail += l;	/* count full message w/ '\0' terminating char */
	} else {
		/* we need to break up the message and "circle" it around */
		char *c;
		int k = G.shbuf->tail + l - G.shbuf->size;	/* count # of bytes we don't fit */

		/* We need to move HEAD! This is always the case since we are going
		 * to "circle" the message.
		 */
		c = memchr(G.shbuf->data + k, '\0', G.shbuf->size - k);

		if (c != NULL) {	/* if we don't have '\0'??? weird!!! */
			/* move head pointer */
			G.shbuf->head = c - G.shbuf->data + 1;

			/* now write the first part of the message */
			strncpy(G.shbuf->data + G.shbuf->tail, msg, l - k - 1);

			/* ALWAYS terminate end of buffer w/ '\0' */
			G.shbuf->data[G.shbuf->size - 1] = '\0';

			/* now write out the rest of the string to the beginning of the buffer */
			strcpy(G.shbuf->data, &msg[l - k - 1]);

			/* we need to place the TAIL at the end of the message */
			G.shbuf->tail = k + 1;
		} else {
			printf
				("Weird! Can't find the terminator token from the beginning??? \n");
			G.shbuf->head = G.shbuf->tail = 0;	/* reset buffer, since it's probably corrupted */
		}

	}
	if (semop(G.s_semid, G.SMwup, 1) == -1) {
		bb_perror_msg_and_die("SMwup");
	}
}

/* Write message to shared mem buffer */
static void log_to_shmem(const char *msg)
{
#if 0

	int old_tail, new_tail;
	int len;

	if (semop(G.s_semid, G.SMwdn, 3) == -1) {
		bb_perror_msg_and_die("SMwdn");
	}

	/* Circular Buffer Algorithm:
	 * --------------------------
	 * tail == position where to store next syslog message.
	 * tail's max value is (shbuf->size - 1)
	 * Last byte of buffer is never used and remains NUL.
	 */

	/* the circular buffer algotithm function is simlpied, without considering the head */
	len = strlen(msg) + 1; /* length with NUL included */
 again:
	old_tail = G.shbuf->tail;
	new_tail = old_tail + len;
	if (new_tail < G.shbuf->size) {
		/* store message, set new tail */
		memcpy(G.shbuf->data + old_tail, msg, len);
		G.shbuf->tail = new_tail;
	} else {
		/* k == available buffer space ahead of old tail */
		int k = G.shbuf->size - old_tail;
		/* copy what fits to the end of buffer, and repeat */
		memcpy(G.shbuf->data + old_tail, msg, k);
		msg += k;
		len -= k;
		G.shbuf->tail = 0;
		goto again;
	}

	if (semop(G.s_semid, G.SMwup, 1) == -1) {
		bb_perror_msg_and_die("SMwup");
	}
#else
	/* 
	 * 此处调用circ_message函数(拷贝自busybox-1.01)，因为要对head指针进行处理
	 */
	circ_message(msg);
#endif
	if (DEBUG)
		printf("tail:%d\n", G.shbuf->tail);
}
#else
#if 0
void ipcsyslog_cleanup(void);
#endif
void ipcsyslog_init(void);
void log_to_shmem(const char *msg);
#endif /* FEATURE_IPC_SYSLOG */

/* Print a message to the log file. */
static void log_locally(time_t now, char *msg, logFile_t *log_file)
{
#ifdef SYSLOGD_WRLOCK
	struct flock fl;
#endif
	int len = strlen(msg);

	if (log_file->fd >= 0) {
		/* Reopen log file every second. This allows admin
		 * to delete the file and not worry about restarting us.
		 * This costs almost nothing since it happens
		 * _at most_ once a second.
		 */
		if (!now)
			now = time(NULL);
		if (G.last_log_time != now) {
			G.last_log_time = now;
			close(log_file->fd);
			goto reopen;
		}
	} else {
 reopen:
		log_file->fd = open(log_file->path, O_WRONLY | O_CREAT
					| O_NOCTTY | O_APPEND | O_NONBLOCK,
					0666);
		if (log_file->fd < 0) {
			/* cannot open logfile? - print to /dev/console then */
			int fd = device_open(DEV_CONSOLE, O_WRONLY | O_NOCTTY | O_NONBLOCK);
			if (fd < 0)
				fd = 2; /* then stderr, dammit */
			full_write(fd, msg, len);
			if (fd != 2)
				close(fd);
			return;
		}
#if ENABLE_FEATURE_ROTATE_LOGFILE
		{
			struct stat statf;
			log_file->isRegular = (fstat(log_file->fd, &statf) == 0 && S_ISREG(statf.st_mode));
			/* bug (mostly harmless): can wrap around if file > 4gb */
			log_file->size = statf.st_size;
		}
#endif
	}

#ifdef SYSLOGD_WRLOCK
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;
	fl.l_type = F_WRLCK;
	fcntl(log_file->fd, F_SETLKW, &fl);
#endif

#if ENABLE_FEATURE_ROTATE_LOGFILE
	if (G.logFileSize && log_file->isRegular && log_file->size > G.logFileSize) {
		if (G.logFileRotate) { /* always 0..99 */
			int i = strlen(log_file->path) + 3 + 1;
			char oldFile[i];
			char newFile[i];
			i = G.logFileRotate - 1;
			/* rename: f.8 -> f.9; f.7 -> f.8; ... */
			while (1) {
				sprintf(newFile, "%s.%d", log_file->path, i);
				if (i == 0) break;
				sprintf(oldFile, "%s.%d", log_file->path, --i);
				/* ignore errors - file might be missing */
				rename(oldFile, newFile);
			}
			/* newFile == "f.0" now */
			rename(log_file->path, newFile);
			/* Incredibly, if F and F.0 are hardlinks, POSIX
			 * _demands_ that rename returns 0 but does not
			 * remove F!!!
			 * (hardlinked F/F.0 pair was observed after
			 * power failure during rename()).
			 * Ensure old file is gone:
			 */
			unlink(log_file->path);
#ifdef SYSLOGD_WRLOCK
			fl.l_type = F_UNLCK;
			fcntl(log_file->fd, F_SETLKW, &fl);
#endif
			close(log_file->fd);
			goto reopen;
		}
		ftruncate(log_file->fd, 0);
	}
	log_file->size +=
#endif
			full_write(log_file->fd, msg, len);
#ifdef SYSLOGD_WRLOCK
	fl.l_type = F_UNLCK;
	fcntl(log_file->fd, F_SETLKW, &fl);
#endif
}

static void parse_fac_prio_20(int pri, char *res20)
{
	const CODE *c_pri, *c_fac;

	c_fac = find_by_val(LOG_FAC(pri) << 3, facilitynames);
	if (c_fac) {
		c_pri = find_by_val(LOG_PRI(pri), prioritynames);
		if (c_pri) {
			snprintf(res20, 20, "%s.%s", c_fac->c_name, c_pri->c_name);
			return;
		}
	}
	snprintf(res20, 20, "<%d>", pri);
}

/* len parameter is used only for "is there a timestamp?" check.
 * NB: some callers cheat and supply len==0 when they know
 * that there is no timestamp, short-circuiting the test. */
static void timestamp_and_log(int pri, char *msg, int len)
{
	char *timestamp;
	time_t now = 0;
	time_t tmpTime = 0;
	struct timespec time1;
	struct timespec time2;
#if ENABLE_FEATURE_REMOTE_LOG
	llist_t *item;
	char line[MAX_READ + 1] = {0};
#endif

#if 0
	/* Jan 18 00:11:22 msg... */
	/* 01234567890123456 */
	if (len < 16 || msg[3] != ' ' || msg[6] != ' '
	 || msg[9] != ':' || msg[12] != ':' || msg[15] != ' '
	) {
		time(&now);
		timestamp = ctime(&now) + 4; /* skip day of week */
	} else {
		now = 0;
		timestamp = msg;
		msg += 16;
	}
	timestamp[15] = '\0';
#else
	/* EAP 自定义的时间戳格式 */
	if (strlen(msg) < 19 || msg[4] != '-' || msg[7] != '-' ||
		msg[10] != ' ' || msg[13] != ':' || msg[16] != ':' || msg[19] != ' ') {

        struct tm* timenow = NULL;
		static char timeStr[20];
		memset(&time1, 0, sizeof(struct timespec));
		memset(&time2, 0, sizeof(struct timespec));
		clock_gettime(CLOCK_REALTIME, &time1);
		clock_gettime(CLOCK_MONOTONIC, &time2);
		now = time1.tv_sec;
		tmpTime = time2.tv_sec;

        memset(timeStr, 0, 20);
        /* time(&now); */
        timenow = localtime(&now);
        sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
               timenow->tm_year+1900, timenow->tm_mon+1, timenow->tm_mday,
               timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

		timestamp = timeStr;
		
	} else {
		timestamp = msg;
		timestamp[19] = '\0';
		msg += 20;
	}
#endif

#if 0
	/* no longer support option '-m', and new log format is defined later */
	if (option_mask32 & OPT_small)
		sprintf(G.printbuf, "%s %s\n", timestamp, msg);
	else {
		char res[20];
		parse_fac_prio_20(pri, res);
		sprintf(G.printbuf, "%s %.64s %s %s\n", timestamp, G.hostname, res, msg);
	}
#endif


#if ENABLE_FEATURE_REMOTE_LOG

	/* only send log to remote log server when loglevel is suitable */
	if (LOG_PRI(pri) <= G.remotelogLevel)
	{
		snprintf(line, sizeof(line), "<%d> %s", pri, msg);

		for (item = G.remoteHosts; item != NULL; item = item->link) {
			remoteHost_t *rh = (remoteHost_t *)item->data;

			if (rh->remoteFD == -1) {
				rh->remoteFD = try_to_resolve_remote(rh);
				if (rh->remoteFD == -1)
					continue;
			}

			/* Send message to remote logger.
			 * On some errors, close and set remoteFD to -1
			 * so that DNS resolution is retried.
			 */
			if (sendto(rh->remoteFD, line, strlen(line),
					MSG_DONTWAIT | MSG_NOSIGNAL,
					&(rh->remoteAddr->u.sa), rh->remoteAddr->len) == -1
			) {
				switch (errno) {
				case ECONNRESET:
				case ENOTCONN: /* paranoia */
				case EPIPE:
					close(rh->remoteFD);
					rh->remoteFD = -1;
					free(rh->remoteAddr);
					rh->remoteAddr = NULL;
				}
			}
		}
	}
#endif
	/* local log format of eap */
	if (option_mask32 & OPT_locallog)
	{
   		char res[20] = {0};
		parse_fac_prio_20(pri, res);
		/* 日志长度的计算：除去时间戳timestamp、hostname、res、消息msg字段的实际长度外，
		 * 还要加上 6 spaces + len(3bytes) + uptime(10bytes) + logTimeFlag(1byte) + \0 + \n
		 * logTimeFlag:指示日志时间是否正确，用于syslog模块纠正日志时间
		 */
		len = strlen(timestamp) + strlen(G.hostname) + strlen(res) + strlen(msg) + 9 + 13;

		snprintf(G.printbuf, len, "%s %s %s %s %3i %10ld %c\n", timestamp, G.hostname, res, msg, len, tmpTime, logTimeFlag);
	}

	/* Log message locally (to file or shared mem) */
#if ENABLE_FEATURE_SYSLOGD_CFG
	{
		bool match = 0;
		logRule_t *rule;
		uint8_t facility = LOG_FAC(pri);
		uint8_t prio_bit = 1 << LOG_PRI(pri);

		for (rule = G.log_rules; rule; rule = rule->next) {
			if (rule->enabled_facility_priomap[facility] & prio_bit) {
				log_locally(now, G.printbuf, rule->file);
				match = 1;
			}
		}
		if (match)
			return;
	}
#endif

	if (LOG_PRI(pri) <= G.logLevel)
	{
#if ENABLE_FEATURE_IPC_SYSLOG
		if ((option_mask32 & OPT_circularlog) && G.shbuf) {
			log_to_shmem(G.printbuf);
			return;
		}
#endif
		log_locally(now, G.printbuf, &G.logFile);
	}
}

static void timestamp_and_log_internal(const char *msg)
{
	/* do not save useless log */
	#if 0
	/* -L, or no -R */
	if (ENABLE_FEATURE_REMOTE_LOG && !(option_mask32 & OPT_locallog))
		return;
	timestamp_and_log(LOG_SYSLOG | LOG_INFO, (char*)msg, 0);
	#endif
}

/* tmpbuf[len] is a NUL byte (set by caller), but there can be other,
 * embedded NULs. Split messages on each of these NULs, parse prio,
 * escape control chars and log each locally. */
static void split_escape_and_log(char *tmpbuf, int len)
{
	char *p = tmpbuf;

	tmpbuf += len;
	while (p < tmpbuf) {
		char c;
		char *q = G.parsebuf;
		int pri = (LOG_USER | LOG_NOTICE);

		if (*p == '<') {
			/* Parse the magic priority number */
			pri = bb_strtou(p + 1, &p, 10);
			if (*p == '>')
				p++;
			if (pri & ~(LOG_FACMASK | LOG_PRIMASK))
				pri = (LOG_USER | LOG_NOTICE);
		}

		while ((c = *p++)) {
			if (c == '\n')
				c = ' ';
			if (!(c & ~0x1f) && c != '\t') {
				*q++ = '^';
				c += '@'; /* ^@, ^A, ^B... */
			}
			*q++ = c;
		}
		*q = '\0';

		/* Now log it */
		timestamp_and_log(pri, G.parsebuf, q - G.parsebuf);
	}
}

#ifdef SYSLOGD_MARK
static void do_mark(int sig)
{
	if (G.markInterval) {
		timestamp_and_log_internal("-- MARK --");
		alarm(G.markInterval);
	}
}
#endif

/* Don't inline: prevent struct sockaddr_un to take up space on stack
 * permanently */
static NOINLINE int create_socket(void)
{
	struct sockaddr_un sunx;
	int sock_fd;
	char *dev_log_name;

#if ENABLE_FEATURE_SYSTEMD
	if (sd_listen_fds() == 1)
		return SD_LISTEN_FDS_START;
#endif

	memset(&sunx, 0, sizeof(sunx));
	sunx.sun_family = AF_UNIX;

	/* Unlink old /dev/log or object it points to. */
	/* (if it exists, bind will fail) */
	//strcpy(sunx.sun_path, "/dev/log");
	//dev_log_name = xmalloc_follow_symlinks("/dev/log");
	/* a unix socket to transform log messages */
	strncpy(sunx.sun_path, TP_PATH_LOG, sizeof(sunx.sun_path));
	dev_log_name = xmalloc_follow_symlinks(TP_PATH_LOG);
	if (dev_log_name) {
		safe_strncpy(sunx.sun_path, dev_log_name, sizeof(sunx.sun_path));
		free(dev_log_name);
	}
	unlink(sunx.sun_path);

	sock_fd = xsocket(AF_UNIX, SOCK_DGRAM, 0);
	xbind(sock_fd, (struct sockaddr *) &sunx, sizeof(sunx));

	//chmod("/dev/log", 0666);
	if (chmod(TP_PATH_LOG, 0666) < 0)
	{
		bb_perror_msg_and_die("failed to set permission on " TP_PATH_LOG);
	}

	return sock_fd;
}

#if ENABLE_FEATURE_REMOTE_LOG
static int try_to_resolve_remote(remoteHost_t *rh)
{
	if (!rh->remoteAddr) {
		unsigned now = monotonic_sec();

		/* Don't resolve name too often - DNS timeouts can be big */
		if ((now - rh->last_dns_resolve) < DNS_WAIT_SEC)
			return -1;
		rh->last_dns_resolve = now;
		rh->remoteAddr = host2sockaddr(rh->remoteHostname, 514);
		if (!rh->remoteAddr)
			return -1;
	}
	return xsocket(rh->remoteAddr->u.sa.sa_family, SOCK_DGRAM, 0);
}
#endif

static void do_syslogd(void) NORETURN;
static void do_syslogd(void)
{
	int sock_fd;
	LOG_DATA logData = {0};
#if 0
#if ENABLE_FEATURE_REMOTE_LOG
	llist_t *item;
#endif
#endif
#if ENABLE_FEATURE_SYSLOGD_DUP
	int last_sz = -1;
	char *last_buf;
	char *recvbuf = G.recvbuf;
#else
#define recvbuf (G.recvbuf)
#endif

	/* Set up signal handlers (so that they interrupt read()) */
	signal_no_SA_RESTART_empty_mask(SIGTERM, record_signo);
	signal_no_SA_RESTART_empty_mask(SIGINT, record_signo);
	//signal_no_SA_RESTART_empty_mask(SIGQUIT, record_signo);
	signal(SIGHUP, SIG_IGN);
#ifdef SYSLOGD_MARK
	signal(SIGALRM, do_mark);
	alarm(G.markInterval);
#endif

	sock_fd = create_socket();
	printf("sockfd is valid?:%d\n", sock_fd);
	if (ENABLE_FEATURE_IPC_SYSLOG && (option_mask32 & OPT_circularlog)) {

		ipcsyslog_init();
	}

	timestamp_and_log_internal("syslogd started: BusyBox v" BB_VER);

	while (!bb_got_signal) {
		ssize_t sz;

#if ENABLE_FEATURE_SYSLOGD_DUP
		last_buf = recvbuf;
		if (recvbuf == G.recvbuf)
			recvbuf = G.recvbuf + MAX_READ;
		else
			recvbuf = G.recvbuf;
#endif
#if 0
 read_again:
#endif
		sz = read(sock_fd, recvbuf, MAX_READ - 1);
		if (sz < 0) {
			if (!bb_got_signal)
				//bb_perror_msg("read from /dev/log");
				bb_perror_msg("read from /var/log");
			break;
		}
		printf("sz = %d, data struct = %d\n", sz, sizeof(logData));
		/* 
		 * 由于我们此处处理msglogd的日志格式，因此Drop trailing '\n' and NULs的处理省略
		 */
#if 0
		/* Drop trailing '\n' and NULs (typically there is one NUL) */
		while (1) {
			if (sz == 0)
				printf("you got an empty log?\n");
				goto read_again;
			/* man 3 syslog says: "A trailing newline is added when needed".
			 * However, neither glibc nor uclibc do this:
			 * syslog(prio, "test")   sends "test\0" to /dev/log,
			 * syslog(prio, "test\n") sends "test\n\0".
			 * IOW: newline is passed verbatim!
			 * I take it to mean that it's syslogd's job
			 * to make those look identical in the log files. */
			if (recvbuf[sz-1] != '\0' && recvbuf[sz-1] != '\n')
				break;
			sz--;
		}
#endif
#if ENABLE_FEATURE_SYSLOGD_DUP
		if ((option_mask32 & OPT_dup) && (sz == last_sz))
			if (memcmp(last_buf, recvbuf, sz) == 0)
				continue;
		last_sz = sz;
#endif
	/* 
	 * 发送给源端server的日志格式需要经过处理，因此不在此处进行日志发送，
	 * 而是放在timestamp_and_log函数中进行
	 */
#if 0
#if ENABLE_FEATURE_REMOTE_LOG
		/* Stock syslogd sends it '\n'-terminated
		 * over network, mimic that */
		recvbuf[sz] = '\n';

		/* We are not modifying log messages in any way before send */
		/* Remote site cannot trust _us_ anyway and need to do validation again */
		for (item = G.remoteHosts; item != NULL; item = item->link) {
			remoteHost_t *rh = (remoteHost_t *)item->data;

			if (rh->remoteFD == -1) {
				rh->remoteFD = try_to_resolve_remote(rh);
				if (rh->remoteFD == -1)
					continue;
			}

			/* Send message to remote logger.
			 * On some errors, close and set remoteFD to -1
			 * so that DNS resolution is retried.
			 */
			if (sendto(rh->remoteFD, recvbuf, sz+1,
					MSG_DONTWAIT | MSG_NOSIGNAL,
					&(rh->remoteAddr->u.sa), rh->remoteAddr->len) == -1
			) {
				switch (errno) {
				case ECONNRESET:
				case ENOTCONN: /* paranoia */
				case EPIPE:
					close(rh->remoteFD);
					rh->remoteFD = -1;
					free(rh->remoteAddr);
					rh->remoteAddr = NULL;
				}
			}
		}
#endif
#endif
		if (!ENABLE_FEATURE_REMOTE_LOG || (option_mask32 & OPT_locallog)) {
			/* 
			 * 按照msglogd和syslogd交互的数据格式解析日志
			 */
			if (sz != sizeof(LOG_DATA))
			{
				puts("incorrect rcv data len\n");
				continue;
			}
			memset(&logData, 0, sizeof(LOG_DATA));
			memcpy(&logData, recvbuf, sizeof(LOG_DATA));
			logTimeFlag = (logData.logTimeFlag == 1) ? LOG_TIME_IS_CORRECT : LOG_TIME_NOT_CORRECT;
			split_escape_and_log(logData.data, logData.dataLen);
			//split_escape_and_log(recvbuf, sz);
		}
	} /* while (!bb_got_signal) */

	timestamp_and_log_internal("syslogd exiting");
	puts("syslogd exiting");
	/*
	 * 为了保证syslog模块读取系统日志时总是能成功，我们不再会在退出时清除共享内存区
	 */
#if 0
	if (ENABLE_FEATURE_IPC_SYSLOG)
		ipcsyslog_cleanup();
#endif
	kill_myself_with_sig(bb_got_signal);
#undef recvbuf
}

int syslogd_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int syslogd_main(int argc UNUSED_PARAM, char **argv)
{
	int opts;
	char OPTION_DECL;
#if ENABLE_FEATURE_REMOTE_LOG
	llist_t *remoteAddrList = NULL;
#endif
	FILE *pidfile;

	INIT_G();

	/* No non-option params, -R can occur multiple times */
	/* busybox-1.20.2支持配置多个 remote log server 但目前EAP仅允许配置一个 */
	opt_complementary = "=0" IF_FEATURE_REMOTE_LOG(":R::");
	opts = getopt32(argv, OPTION_STR, OPTION_PARAM);
#if ENABLE_FEATURE_REMOTE_LOG
	while (remoteAddrList) {
		remoteHost_t *rh = xzalloc(sizeof(*rh));
		rh->remoteHostname = llist_pop(&remoteAddrList);
		rh->remoteFD = -1;
		rh->last_dns_resolve = monotonic_sec() - DNS_WAIT_SEC - 1;
		llist_add_to(&G.remoteHosts, rh);
	}
#endif

#ifdef SYSLOGD_MARK
	if (opts & OPT_mark) // -m
		G.markInterval = xatou_range(opt_m, 0, INT_MAX/60) * 60;
#endif
	//if (opts & OPT_nofork) // -n
	//if (opts & OPT_outfile) // -O
	if (opts & OPT_loglevel) // -l
	{
		G.logLevel = xatou_range(opt_l, 1, 8);
	}
	//if (opts & OPT_small) // -S
#if ENABLE_FEATURE_ROTATE_LOGFILE
	if (opts & OPT_filesize) // -s
		G.logFileSize = xatou_range(opt_s, 0, INT_MAX/1024) * 1024;
	if (opts & OPT_rotatecnt) // -b
		G.logFileRotate = xatou_range(opt_b, 0, 99);
#endif
#if ENABLE_FEATURE_IPC_SYSLOG
	if (opt_C) // -Cn
		G.shm_size = xatoul_range(opt_C, 4, INT_MAX/1024) * 1024;
	/* 
	 * 特殊处理:当启动参数带有"-C: -l:"时，等同于"-C: -L -l:"，需要将日志保存到本地
	 */
	if ((opts & OPT_circularlog) && (opts & OPT_loglevel))
	{
		option_mask32 |= OPT_locallog;
	}
#endif
	/* If they have not specified remote logging, then log locally */
	if (ENABLE_FEATURE_REMOTE_LOG && !(opts & OPT_remotelog)) // -R
		option_mask32 |= OPT_locallog;
#if ENABLE_FEATURE_SYSLOGD_CFG
	parse_syslogdcfg(opt_f);
#endif
#if ENABLE_FEATURE_REMOTE_LOG
	if (opts & OPT_remoteloglevel)	// -r
	{
		G.remotelogLevel = xatou_range(opt_l, 1, 8);
	}
#endif
	/* Store away localhost's name before the fork */
	G.hostname = safe_gethostname();
	*strchrnul(G.hostname, '.') = '\0';

	if (!(opts & OPT_nofork)) {
		bb_daemonize_or_rexec(DAEMON_CHDIR_ROOT, argv);
	}
	//umask(0); - why??

	if ((pidfile = fopen (l_syslogPidFile, "w")) != NULL)
	{
		fprintf (pidfile, "%d\n", getpid ());
		(void) fclose (pidfile);
	}

	do_syslogd();
	/* return EXIT_SUCCESS; */
}

/* Clean up. Needed because we are included from syslogd_and_logger.c */
#undef DEBUG
#undef SYSLOGD_MARK
#undef SYSLOGD_WRLOCK
#undef G
#undef GLOBALS
#undef INIT_G
#undef OPTION_STR
#undef OPTION_DECL
#undef OPTION_PARAM
