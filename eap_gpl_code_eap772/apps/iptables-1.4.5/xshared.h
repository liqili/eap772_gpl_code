#ifndef IPTABLES_XSHARED_H
#define IPTABLES_XSHARED_H 1
#include <stdbool.h>

struct xtables_rule_match;
struct xtables_target;

extern void print_extension_helps(const struct xtables_target *,
	const struct xtables_rule_match *);
extern bool xtables_lock(bool wait);

#endif /* IPTABLES_XSHARED_H */
