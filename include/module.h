#ifndef MODULE_H
#define MODULE_H

#include "types.h"

/* Module structure */
typedef struct module {
    char name[64];
    void* init_func;
    void* exit_func;
    u32 refcount;
    bool loaded;
    struct module* next;
} module_t;

/* Module operations */
typedef struct {
    int (*init)(void);
    void (*exit)(void);
} module_ops_t;

/* Initialize module system */
void module_init(void);

/* Load module */
int module_load(const char* name, module_ops_t* ops);

/* Unload module */
int module_unload(const char* name);

/* Get module */
module_t* module_get(const char* name);

/* Put module */
void module_put(const char* name);

/* Module macros */
#define MODULE_INIT(name, init_func, exit_func) \
    module_ops_t name##_ops = { \
        .init = init_func, \
        .exit = exit_func \
    }; \
    static void __attribute__((constructor)) name##_register(void) { \
        module_load(#name, &name##_ops); \
    }

#endif /* MODULE_H */
