#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "make_flash.h"
#include "md5_interface.h"
#include "cJSON.h"

#include <stdint.h>

SOFT_VER_STRUCT softverInfo;
struct tm now;

int swVer1, swVer2, swVer3;
int hardVer;
unsigned char softver[16] = {0};
#ifdef CONFIG_BOOT_VERSION_SUPPORT
int bootVer = 0;
#endif /* CONFIG_BOOT_VERSION_SUPPORT */

char softversionPath[NM_LONG_STRLEN] = {0};
char addhardPath[NM_LONG_STRLEN] = {0};
char configPath[NM_LONG_STRLEN] = {0};

int debugOn = TRUE;

#define DEBUG_PRINTF(fmt, args...) \
    do {\
        if (debugOn)\
            printf(fmt, ##args);\
    }while(0)

static void show_usage()
{
    printf(
"usage: gen_softver [OPTIIONS]\n\n"
"   -o, --output=FILE               Output Filename\n"
"   -s, --softver                   Software version in format \"1-0-0\"\n"
"   -a, --addhardver                hardware version\n"
"   -d, --debug                     Show debug infomation\n"
"   -h, --help                      Show this message\n");
}

static int hex2str(unsigned char *hexBuf, int hexLen, unsigned char* strBuf)
{
	int ret = -1;
	int i = 0;
	unsigned char temp_hex_str[4];

	if (hexBuf == NULL || strBuf == NULL)
	{
		goto out;
	}

	for (i = 0; i < hexLen; i++)
	{
		memset(temp_hex_str, 0, 4);
		sprintf(temp_hex_str, "%02X", hexBuf[i]);
		strcat(strBuf, temp_hex_str);
	}

	ret = 0;
out:
	return ret;
}

static int str2hex(unsigned char *hexBuf, unsigned char* strBuf)
{
	int ret = -1;
	int i = 0;
	unsigned char* strPtr;
	unsigned char strArr[3] = {'\0'};
	
	if (hexBuf == NULL || strBuf == NULL)
	{
		goto out;
	}

	strPtr = strBuf;
	while (*strPtr != '\0')
	{
		strArr[0] = *strPtr;
		strArr[1] = *(++strPtr);
		hexBuf[i] = (unsigned char)strtoul(strArr, NULL, 16);
		i++;
		strPtr++;
	}

	ret = 0;
out:
	return ret;
}
static int strUpper(unsigned char strBuf[])
{
	int strLen = strlen(strBuf);
	int index = 0;

	for(index = 0; index < strLen; index++)
	{
		if(strBuf[index] >= 'a' && strBuf[index] <= 'z')
		{
			strBuf[index] -= 32;
		}
	}
	return 0;
}

static void strIgnrSpace(char *pStr, int size)
{
    int index = 0, num = 0;
    char tmp[NM_LONG_STRLEN] = {0};
    char *pCur;

    if (NULL == pStr)
    {
        return;
    }

    for (num = 0, pCur = pStr; num < size && '\0' != *pCur; num ++, pCur ++)
    {
        if ((' ' != *pCur) && ('\t' != *pCur) && ('\r' != *pCur) && ('\n' != *pCur))
        {
            tmp[index++] = *pCur;
        }
    }
    tmp[index++] = '\0';

    memcpy(pStr, tmp, index);

    return;
}


static int parseConfigFile(char *pFile)
{
    int ret = 0;
    int index = 0, noused = 0;
    int num_read = 0;
    size_t len = 0;
    char* line = 0;	
    FILE *ptn_file = 0;
    int assigns;
    struct timespec curTime;
    struct tm* tm;
    long actualsize = 0;

	if (!pFile)
	{
		printf("pFile is NULL\n");
		return -1;
	}

    ptn_file = fopen(pFile, "r");
    
    if (!ptn_file)
    {
        printf("failed to open partition file.\n");
        return -1;
    }
        
    while ((num_read = getline(&line, &len, ptn_file)) != -1)
    {
        strIgnrSpace(line, num_read);

		/* get soft_version */
        assigns = sscanf(line, "soft_version=%s",softver);
        if (assigns == 1)
        {
			goto FREE;
		}

		/* get hardver */
		assigns = sscanf(line, "additional_hw_version=%d",&hardVer);
        if (assigns == 1)
        {
			goto FREE;
		}

#ifdef CONFIG_BOOT_VERSION_SUPPORT
		/* get bootVer */
		assigns = sscanf(line, "boot_version=%d",&bootVer);
		if (assigns == 1)
		{
			goto FREE;
		}
#endif /* CONFIG_BOOT_VERSION_SUPPORT */
FREE:
        free(line);
        line = NULL;
    }
        
    fclose(ptn_file);

    return ret;
}


static int parseSoftVerInfo(unsigned char *softverStr)
{
    int assigns;
    struct tm *timenow = NULL;
    char strDate[12];
    char *chgTmpPtr;
	time_t time_now;

    assigns = sscanf(softverStr, "%d-%d-%d", &swVer1, &swVer2, &swVer3);
    if (3 != assigns)
    {
        printf("-s --softver    Software version in format \"1-0-0\"\n");
        return -1;
    }

    softverInfo.sn = (0xff << 24 | (swVer1 & 0xff) << 16 | (swVer2 & 0xff) << 8 | (swVer3 & 0xff));
    softverInfo.sn = htonl(softverInfo.sn);
    time(&time_now);
    timenow = localtime(&time_now);
    softverInfo.releaseTime = 3600 * timenow->tm_hour + 60 * timenow->tm_min + timenow->tm_sec;
    sprintf(strDate, "0x%04d%02d%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday);
    softverInfo.buildDate = strtoul(strDate, &chgTmpPtr, 0);
	softverInfo.buildDate = htonl(softverInfo.buildDate);
	softverInfo.releaseTime = htonl(softverInfo.releaseTime);
	softverInfo.additionalHardVer = htonl(hardVer);
#ifdef CONFIG_BOOT_VERSION_SUPPORT
	softverInfo.bootVer = htonl(bootVer);
#endif /* CONFIG_BOOT_VERSION_SUPPORT */

	return 0;
}

static int writeSoftver(const char *path)
{
    FILE *fp = fopen(path, "wb+");
    int ret = -1;
    if (fp == NULL)
    {
        printf("fail to open file(%s).\n", path);
        goto LEAVE;
    }

    if (fwrite(&softverInfo, sizeof(SOFT_VER_STRUCT), 1, fp) != 1)
    {
		printf("fail to write softversion\n");
        goto LEAVE;
	}
	
    ret = 0;
    
LEAVE:
    if (fp)
    {
        fclose(fp);
    }
    return ret;
}

static int writeAddHard(const char *path)
{
    FILE *fp = fopen(path, "wb+");
    int ret = -1;
    if (fp == NULL)
    {
        printf("fail to open file(%s).\n", path);
        goto LEAVE;
    }

    if (fwrite(&softverInfo.additionalHardVer, sizeof(uint32_t), 1, fp) != 1)
    {
		printf("fail to write softversion\n");
        goto LEAVE;
	}
	
    ret = 0;
    
LEAVE:
    if (fp)
    {
        fclose(fp);
    }
    return ret;
}


int main(int argc, char *argv[])
{
    int c = 0;

    static const struct option arg_options[] = {
        {"output",          required_argument,  0, 'o'},
		{"addhard",         required_argument,  0, 'a'},
        {"config",          required_argument,  0, 'c'},
        {"debug",           no_argument,        0, 'd'},
        {"help",            no_argument,        0, 'h'},
        {0,0,0,0}
    };

    while(1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "o:c:a:dh", arg_options, &option_index);
        if (c == -1) break;
            
        switch(c) {
        case 'o':
            strncpy(softversionPath, optarg, 
                strlen(optarg) > NM_LONG_STRLEN?NM_LONG_STRLEN:strlen(optarg));
            break;
		case 'a':
            strncpy(addhardPath, optarg, 
                strlen(optarg) > NM_LONG_STRLEN?NM_LONG_STRLEN:strlen(optarg));
            break;
        case 'c':
            strncpy(configPath, optarg, 
                strlen(optarg) > NM_LONG_STRLEN?NM_LONG_STRLEN:strlen(optarg));
            break;
        case 'd':
            debugOn = TRUE;
            break;
        case 'h':
            show_usage();
            exit(0);
            break;
        }
    }

	if (0 != parseConfigFile(configPath))
	{
		printf("parse config failed");
		return 0;
	}

	/* parse soft version */
    if (parseSoftVerInfo(softver) < 0)
    {
        printf("parse soft version info error.\n");
        return 0;
    }
    DEBUG_PRINTF("parse soft version done.\n");

    if (writeSoftver(softversionPath) < 0)
    {
        printf("write software version error.\n");
        return 0;
    }
    DEBUG_PRINTF("write software verison done.\n");

	if (writeAddHard(addhardPath) < 0)
    {
        printf("write additional hard version error.\n");
        return 0;
    }
    DEBUG_PRINTF("write additional hard version done.\n");

    return 0;
}
