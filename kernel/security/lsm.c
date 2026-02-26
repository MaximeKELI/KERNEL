#include "lsm.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

static lsm_module_t* lsm_modules = NULL;
static spinlock_t lsm_lock = SPINLOCK_INIT;

void lsm_init(void) {
    DEBUG_INFO("LSM (Linux Security Modules) framework initialized");
}

int lsm_register_module(lsm_module_t* module) {
    if (!module || !module->name) return -1;
    
    spinlock_lock(&lsm_lock);
    module->enabled = false;
    module->next = lsm_modules;
    lsm_modules = module;
    spinlock_unlock(&lsm_lock);
    
    DEBUG_INFO("LSM module registered: %s", module->name);
    return 0;
}

int lsm_call_hook(u32 hook, void* data) {
    if (hook >= 16) return 0;
    
    spinlock_lock(&lsm_lock);
    
    lsm_module_t* module = lsm_modules;
    while (module) {
        if (module->enabled && module->hooks[hook]) {
            int ret = module->hooks[hook](hook, data);
            if (ret != 0) {
                spinlock_unlock(&lsm_lock);
                return ret;
            }
        }
        module = module->next;
    }
    
    spinlock_unlock(&lsm_lock);
    return 0;
}

int lsm_enable(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&lsm_lock);
    
    lsm_module_t* module = lsm_modules;
    while (module) {
        if (strcmp(module->name, name) == 0) {
            module->enabled = true;
            spinlock_unlock(&lsm_lock);
            DEBUG_INFO("LSM module enabled: %s", name);
            return 0;
        }
        module = module->next;
    }
    
    spinlock_unlock(&lsm_lock);
    return -1;
}

int lsm_disable(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&lsm_lock);
    
    lsm_module_t* module = lsm_modules;
    while (module) {
        if (strcmp(module->name, name) == 0) {
            module->enabled = false;
            spinlock_unlock(&lsm_lock);
            DEBUG_INFO("LSM module disabled: %s", name);
            return 0;
        }
        module = module->next;
    }
    
    spinlock_unlock(&lsm_lock);
    return -1;
}
