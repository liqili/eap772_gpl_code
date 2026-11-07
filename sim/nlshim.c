/*
 * nlshim.so - LD_PRELOAD stand-in for TP-Link's private kernel netlink families in a
 * qemu-user container (no portal.ko, vlan_manage.ko, ... and qemu only forwards a few
 * netlink protocols). When socket(AF_NETLINK, ...) fails, return one end of an AF_UNIX
 * datagram socketpair instead: bind/connect/setsockopt succeed, sends are swallowed,
 * and nothing is ever received, as if the kernel module were loaded but silent.
 * Also redirects uclited's hostname write (see fopen below).
 * Build on the host: sim/build-nlshim.sh (sim/make-image.sh runs it). Simulation only; never
 * part of the firmware. Same shim as the F65 tree's sim/nlshim.c.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#define MAXFD 4096
static int peer[MAXFD];	/* fake fd -> its socketpair peer + 1, 0 = not fake */

#define REAL(name) static __typeof__(name) *real_##name; \
	if (!real_##name) real_##name = (__typeof__(name) *)dlsym(RTLD_NEXT, #name)

static int is_fake(int fd) { return fd >= 0 && fd < MAXFD && peer[fd]; }

int socket(int domain, int type, int proto)
{
	REAL(socket);
	int fd = real_socket(domain, type, proto);
	if (fd >= 0 || domain != AF_NETLINK)
		return fd;
	int sv[2];
	if (socketpair(AF_UNIX, SOCK_DGRAM | (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)), 0, sv) < 0 || sv[0] >= MAXFD)
		return -1;
	peer[sv[0]] = sv[1] + 1;
	fprintf(stderr, "[nlshim] netlink proto %d unavailable, fake socket fd %d\n", proto, sv[0]);
	return sv[0];
}

int bind(int fd, const struct sockaddr *a, socklen_t l) { REAL(bind); return is_fake(fd) ? 0 : real_bind(fd, a, l); }
int connect(int fd, const struct sockaddr *a, socklen_t l) { REAL(connect); return is_fake(fd) ? 0 : real_connect(fd, a, l); }
int setsockopt(int fd, int lv, int o, const void *v, socklen_t l)
{ REAL(setsockopt); return is_fake(fd) ? 0 : real_setsockopt(fd, lv, o, v, l); }

int getsockname(int fd, struct sockaddr *a, socklen_t *l)
{
	REAL(getsockname);
	if (!is_fake(fd))
		return real_getsockname(fd, a, l);
	struct sockaddr_nl nl = { .nl_family = AF_NETLINK, .nl_pid = (unsigned)getpid() };
	if (a && l) { memcpy(a, &nl, *l < sizeof(nl) ? *l : sizeof(nl)); *l = sizeof(nl); }
	return 0;
}

ssize_t sendto(int fd, const void *b, size_t n, int f, const struct sockaddr *a, socklen_t l)
{ REAL(sendto); return is_fake(fd) ? (ssize_t)n : real_sendto(fd, b, n, f, a, l); }
ssize_t send(int fd, const void *b, size_t n, int f) { REAL(send); return is_fake(fd) ? (ssize_t)n : real_send(fd, b, n, f); }
ssize_t sendmsg(int fd, const struct msghdr *m, int f)
{
	REAL(sendmsg);
	if (!is_fake(fd))
		return real_sendmsg(fd, m, f);
	ssize_t n = 0;
	for (size_t i = 0; i < m->msg_iovlen; i++)
		n += m->msg_iov[i].iov_len;
	return n;
}

/* Receives fall through to the (never written) socketpair end: they block, or return
 * EAGAIN on non-blocking sockets, and select/poll/epoll never report them readable. */

int close(int fd)
{
	REAL(close);
	if (is_fake(fd)) { real_close(peer[fd] - 1); peer[fd] = 0; }
	return real_close(fd);
}

/*
 * uclited's initSysInfo() writes the hostname with fopen("/proc/sys/kernel/hostname", "w").
 * /proc/sys is read-only in an unprivileged container, and that failure makes uclited skip
 * loading every config module ("Failed to init runtime ptnstruct!"), MANAGE_USERS included,
 * so the first login dereferences a NULL account record. Send the write to a plain file.
 */
#define HOSTNAME_PROC "/proc/sys/kernel/hostname"
#define HOSTNAME_SIM "/tmp/sim-hostname"
FILE *fopen(const char *path, const char *mode)
{
	REAL(fopen);
	FILE *f = real_fopen(path, mode);
	if (!f && path && mode && mode[0] != 'r' && !strcmp(path, HOSTNAME_PROC)) {
		fprintf(stderr, "[nlshim] %s is read-only, writing %s instead\n", HOSTNAME_PROC, HOSTNAME_SIM);
		f = real_fopen(HOSTNAME_SIM, mode);
	}
	return f;
}
