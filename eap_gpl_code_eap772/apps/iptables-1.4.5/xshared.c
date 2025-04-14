#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <xtables.h>
#include "xshared.h"
#define XT_SOCKET_NAME  "xtables"
#define XT_SOCKET_LEN   8

/*
 * Print out any special helps. A user might like to be able to add a --help
 * to the commandline, and see expected results. So we call help for all
 * specified matches and targets.
 */
void print_extension_helps(const struct xtables_target *t,
    const struct xtables_rule_match *m)
{
	for (; t != NULL; t = t->next) {
		if (t->used) {
			printf("\n");
			if (t->help == NULL)
				printf("%s does not take any options\n",
				       t->name);
			else
				t->help();
		}
	}
	for (; m != NULL; m = m->next) {
		printf("\n");
		if (m->match->help == NULL)
			printf("%s does not take any options\n",
			       m->match->name);
		else
			m->match->help();
	}
}

#define xtables_offsetof(TYPE, MEMBER) ((size_t)&((TYPE*)0)->MEMBER)

bool xtables_lock(bool wait)
{
    int i = 0, ret, xt_socket;
    struct sockaddr_un xt_addr;

    memset(&xt_addr, 0, sizeof(xt_addr));
    xt_addr.sun_family = AF_UNIX;
    strcpy(xt_addr.sun_path+1, XT_SOCKET_NAME);
    xt_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    /* If we can't even create a socket, fall back to prior (lockless) behavior */
    if (xt_socket < 0)
        return true;

    while (1) {
        ret = bind(xt_socket, (struct sockaddr*)&xt_addr,
                   xtables_offsetof(struct sockaddr_un, sun_path)+XT_SOCKET_LEN);
        if (ret == 0)
        {
            return true;
        }
        else if (wait == false)
        {
            return false;
        }

        if (++i % 2 == 0)
            fprintf(stderr, "Another app is currently holding the xtables lock; "
                    "waiting for it to exit...\n");
        sleep(1);
    }

    return true;
}

