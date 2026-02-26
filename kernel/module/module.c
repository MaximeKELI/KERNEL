#include "module.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_MODULES 64

static module_t modules[MAX_MODULES];
static module_t* module_list = NULL;
static spinlock_t module_lock = SPINLOCK_INIT;
static u32 next_module = 0;

void module_init(void) {
    memset(modules, 0, sizeof(modules));
    DEBUG_INFO("Module system initialized");
}

int module_load(const char* name, module_ops_t* ops) {
    if (!name || !ops) {
        DEBUG_ERROR("Invalid module load parameters");
        return -1;
    }
    
    if (next_module >= MAX_MODULES) {
        DEBUG_ERROR("Maximum modules reached");
        return -1;
    }
    
    spinlock_lock(&module_lock);
    
    /* Check if already loaded */
    module_t* mod = module_list;
    while (mod) {
        if (strcmp(mod->name, name) == 0) {
            mod->refcount++;
            spinlock_unlock(&module_lock);
            DEBUG_INFO("Module %s already loaded (refcount: %u)", name, mod->refcount);
            return 0;
        }
        mod = mod->next;
    }
    
    /* Allocate new module */
    module_t* new_mod = &modules[next_module++];
    strncpy(new_mod->name, name, sizeof(new_mod->name) - 1);
    new_mod->init_func = (void*)ops->init;
    new_mod->exit_func = (void*)ops->exit;
    new_mod->refcount = 1;
    new_mod->loaded = false;
    
    /* Call init function */
    if (ops->init) {
        int ret = ops->init();
        if (ret < 0) {
            DEBUG_ERROR("Module %s init failed: %d", name, ret);
            memset(new_mod, 0, sizeof(module_t));
            spinlock_unlock(&module_lock);
            return -1;
        }
    }
    
    new_mod->loaded = true;
    new_mod->next = module_list;
    module_list = new_mod;
    
    spinlock_unlock(&module_lock);
    
    DEBUG_INFO("Module %s loaded successfully", name);
    return 0;
}

int module_unload(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&module_lock);
    
    module_t** prev = &module_list;
    module_t* mod = module_list;
    
    while (mod) {
        if (strcmp(mod->name, name) == 0) {
            if (mod->refcount > 1) {
                mod->refcount--;
                spinlock_unlock(&module_lock);
                DEBUG_INFO("Module %s refcount decreased to %u", name, mod->refcount);
                return 0;
            }
            
            /* Call exit function */
            if (mod->exit_func && mod->loaded) {
                void (*exit_func)(void) = (void (*)(void))mod->exit_func;
                exit_func();
            }
            
            *prev = mod->next;
            memset(mod, 0, sizeof(module_t));
            
            spinlock_unlock(&module_lock);
            DEBUG_INFO("Module %s unloaded", name);
            return 0;
        }
        prev = &mod->next;
        mod = mod->next;
    }
    
    spinlock_unlock(&module_lock);
    DEBUG_WARN("Module %s not found for unload", name);
    return -1;
}

module_t* module_get(const char* name) {
    if (!name) return NULL;
    
    spinlock_lock(&module_lock);
    
    module_t* mod = module_list;
    while (mod) {
        if (strcmp(mod->name, name) == 0) {
            mod->refcount++;
            spinlock_unlock(&module_lock);
            return mod;
        }
        mod = mod->next;
    }
    
    spinlock_unlock(&module_lock);
    return NULL;
}

void module_put(const char* name) {
    if (!name) return;
    
    spinlock_lock(&module_lock);
    
    module_t* mod = module_list;
    while (mod) {
        if (strcmp(mod->name, name) == 0) {
            if (mod->refcount > 0) {
                mod->refcount--;
            }
            spinlock_unlock(&module_lock);
            return;
        }
        mod = mod->next;
    }
    
    spinlock_unlock(&module_lock);
}
