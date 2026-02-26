#ifndef MAC_H
#define MAC_H

#include "types.h"

/* MAC types */
#define MAC_TYPE_NONE    0
#define MAC_TYPE_SELINUX 1
#define MAC_TYPE_APPARMOR 2
#define MAC_TYPE_SMACK   3

/* Security context */
typedef struct security_context {
    char* label;
    u32 type;
    void* data;
} security_context_t;

/* Initialize MAC */
void mac_init(void);

/* Set security context */
int mac_set_context(u64 pid, security_context_t* context);

/* Get security context */
int mac_get_context(u64 pid, security_context_t* context);

/* Check permission */
int mac_check_permission(security_context_t* subject, security_context_t* object, u32 permission);

/* Enable MAC type */
int mac_enable(u32 type);

#endif /* MAC_H */
