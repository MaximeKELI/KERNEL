#ifndef LSM_H
#define LSM_H

#include "types.h"

/* LSM hook types */
#define LSM_HOOK_FILE_OPEN     0
#define LSM_HOOK_FILE_PERM     1
#define LSM_HOOK_INODE_CREATE  2
#define LSM_HOOK_TASK_ALLOC    3
#define LSM_HOOK_SOCKET_CREATE 4

/* LSM hook function */
typedef int (*lsm_hook_func_t)(u32 hook, void* data);

/* LSM module */
typedef struct lsm_module {
    char name[32];
    lsm_hook_func_t hooks[16];
    bool enabled;
    struct lsm_module* next;
} lsm_module_t;

/* Initialize LSM framework */
void lsm_init(void);

/* Register LSM module */
int lsm_register_module(lsm_module_t* module);

/* Call LSM hooks */
int lsm_call_hook(u32 hook, void* data);

/* Enable/disable LSM module */
int lsm_enable(const char* name);
int lsm_disable(const char* name);

#endif /* LSM_H */
