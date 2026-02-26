#include "livepatch.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "interrupt.h"

static livepatch_t* livepatch_list = NULL;
static spinlock_t livepatch_lock = SPINLOCK_INIT;

void livepatch_init(void) {
    DEBUG_INFO("Live patching system initialized");
}

int livepatch_register(livepatch_t* patch) {
    if (!patch || !patch->name || !patch->old_func || !patch->new_func) {
        return -1;
    }
    
    spinlock_lock(&livepatch_lock);
    patch->enabled = false;
    patch->next = livepatch_list;
    livepatch_list = patch;
    spinlock_unlock(&livepatch_lock);
    
    DEBUG_INFO("Live patch registered: %s", patch->name);
    return 0;
}

int livepatch_apply(livepatch_t* patch) {
    if (!patch) return -1;
    
    /* Would replace function pointer */
    /* For now, just mark as applied */
    DEBUG_INFO("Live patch applied: %s", patch->name);
    return 0;
}

int livepatch_enable(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&livepatch_lock);
    
    livepatch_t* patch = livepatch_list;
    while (patch) {
        if (strcmp(patch->name, name) == 0) {
            if (!patch->enabled) {
                livepatch_apply(patch);
                patch->enabled = true;
                spinlock_unlock(&livepatch_lock);
                DEBUG_INFO("Live patch enabled: %s", name);
                return 0;
            }
        }
        patch = patch->next;
    }
    
    spinlock_unlock(&livepatch_lock);
    return -1;
}

int livepatch_disable(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&livepatch_lock);
    
    livepatch_t* patch = livepatch_list;
    while (patch) {
        if (strcmp(patch->name, name) == 0) {
            if (patch->enabled) {
                patch->enabled = false;
                /* Would restore old function */
                spinlock_unlock(&livepatch_lock);
                DEBUG_INFO("Live patch disabled: %s", name);
                return 0;
            }
        }
        patch = patch->next;
    }
    
    spinlock_unlock(&livepatch_lock);
    return -1;
}
