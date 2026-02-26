#ifndef APPARMOR_H
#define APPARMOR_H

#include "types.h"
#include "process.h"

/* AppArmor modes */
#define APPARMOR_MODE_DISABLED  0
#define APPARMOR_MODE_COMPLAIN  1
#define APPARMOR_MODE_ENFORCE  2

/* AppArmor profile */
typedef struct apparmor_profile {
    char name[256];
    u32 mode;
    u64 permissions;
    char* rules;
    size_t rules_size;
    struct apparmor_profile* next;
} apparmor_profile_t;

/* Initialize AppArmor */
int apparmor_init(void);

/* Load profile */
int apparmor_load_profile(const char* name, const char* rules, size_t rules_size);

/* Unload profile */
int apparmor_unload_profile(const char* name);

/* Set process profile */
int apparmor_set_profile(u64 pid, const char* profile_name);

/* Check permission */
int apparmor_check_permission(process_t* proc, const char* path, u32 operation);

/* Get profile for process */
const char* apparmor_get_profile(u64 pid);

#endif /* APPARMOR_H */
