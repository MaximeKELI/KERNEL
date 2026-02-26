#ifndef SELINUX_H
#define SELINUX_H

#include "types.h"
#include "process.h"

/* SELinux modes */
#define SELINUX_DISABLED  0
#define SELINUX_PERMISSIVE 1
#define SELINUX_ENFORCING 2

/* SELinux security context */
typedef struct selinux_context {
    char user[32];
    char role[32];
    char type[32];
    char level[32];
} selinux_context_t;

/* Initialize SELinux */
int selinux_init(void);

/* Set enforcing mode */
int selinux_set_enforcing(bool enforcing);

/* Get enforcing mode */
bool selinux_get_enforcing(void);

/* Set process context */
int selinux_set_context(u64 pid, const selinux_context_t* context);

/* Get process context */
int selinux_get_context(u64 pid, selinux_context_t* context);

/* Check permission */
int selinux_check_permission(process_t* proc, const char* target, u32 operation);

/* Load policy */
int selinux_load_policy(const void* policy, size_t size);

#endif /* SELINUX_H */
