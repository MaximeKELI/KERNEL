#include "apparmor.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"

#define MAX_PROFILES 128

static apparmor_profile_t* profiles = NULL;
static u32 apparmor_mode = APPARMOR_MODE_DISABLED;
static spinlock_t apparmor_lock = SPINLOCK_INIT;

int apparmor_init(void) {
    apparmor_mode = APPARMOR_MODE_DISABLED;
    DEBUG_INFO("%s", "AppArmor security module initialized");
    return 0;
}

int apparmor_load_profile(const char* name, const char* rules, size_t rules_size) {
    VALIDATE_STRING(name, 256);
    VALIDATE_PTR_RET(rules, -1);
    
    spinlock_lock(&apparmor_lock);
    
    /* Check if profile exists */
    apparmor_profile_t* profile = profiles;
    while (profile) {
        if (strcmp(profile->name, name) == 0) {
            spinlock_unlock(&apparmor_lock);
            DEBUG_WARN("Profile already exists: %s", name);
            return -1;
        }
        profile = profile->next;
    }
    
    /* Create new profile */
    profile = (apparmor_profile_t*)kmalloc(sizeof(apparmor_profile_t));
    if (!profile) {
        spinlock_unlock(&apparmor_lock);
        return -1;
    }
    
    strncpy(profile->name, name, sizeof(profile->name) - 1);
    profile->name[sizeof(profile->name) - 1] = '\0';
    profile->mode = APPARMOR_MODE_ENFORCE;
    profile->permissions = 0;
    profile->rules_size = rules_size;
    profile->rules = (char*)kmalloc(rules_size);
    if (!profile->rules) {
        kfree(profile);
        spinlock_unlock(&apparmor_lock);
        return -1;
    }
    memcpy(profile->rules, rules, rules_size);
    profile->next = profiles;
    profiles = profile;
    
    spinlock_unlock(&apparmor_lock);
    
    DEBUG_INFO("AppArmor profile loaded: name=%s, size=%u", name, (u32)rules_size);
    return 0;
}

int apparmor_unload_profile(const char* name) {
    VALIDATE_STRING(name, 256);
    
    spinlock_lock(&apparmor_lock);
    
    apparmor_profile_t** prev = &profiles;
    apparmor_profile_t* profile = profiles;
    
    while (profile) {
        if (strcmp(profile->name, name) == 0) {
            *prev = profile->next;
            kfree(profile->rules);
            kfree(profile);
            
            spinlock_unlock(&apparmor_lock);
            DEBUG_INFO("AppArmor profile unloaded: name=%s", name);
            return 0;
        }
        prev = &profile->next;
        profile = profile->next;
    }
    
    spinlock_unlock(&apparmor_lock);
    return -1;
}

int apparmor_set_profile(u64 pid, const char* profile_name) {
    VALIDATE_STRING(profile_name, 256);
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            /* Would store profile name in process structure */
            DEBUG_INFO("AppArmor profile set: pid=%u, profile=%s", pid, profile_name);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int apparmor_check_permission(process_t* proc, const char* path, u32 operation) {
    VALIDATE_PTR_RET(proc, -1);
    VALIDATE_STRING(path, 256);
    
    if (apparmor_mode == APPARMOR_MODE_DISABLED) {
        return 0; /* Allow */
    }
    
    /* Would check profile rules */
    /* Simplified: allow for now */
    
    (void)operation;
    return 0;
}

const char* apparmor_get_profile(u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            /* Would return profile name from process */
            return "unconfined";
        }
        proc = proc->next;
    }
    
    return NULL;
}
