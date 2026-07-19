#include "capabilities.h"
#include "process.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"

void capabilities_init(void) {
    DEBUG_INFO("Capabilities system initialized");
}

/* Minimal capabilities for root (principle of least privilege) */
/* Root gets only essential capabilities, others must be explicitly granted */
static const u64 root_minimal_caps = 
    (1ULL << 0) |   /* CAP_CHOWN - Change ownership */
    (1ULL << 3) |   /* CAP_FOWNER - Bypass permission checks */
    (1ULL << 4) |   /* CAP_FSETID - Set file setuid */
    (1ULL << 6) |   /* CAP_SETGID - Set group ID */
    (1ULL << 7) |   /* CAP_SETUID - Set user ID */
    (1ULL << 21);   /* CAP_SYS_ADMIN - System administration */

bool capable(int cap) {
    if (cap < 0 || cap > CAP_LAST_CAP) return false;
    
    process_t* proc = process_current();
    if (!proc) return false;
    
    /* Root has minimal capabilities by default (principle of least privilege) */
    if (proc->uid == 0) {
        /* Check if capability is in minimal set */
        if (root_minimal_caps & (1ULL << cap)) {
            return true;
        }
        /* For other capabilities, check if explicitly granted */
        cap_t* caps = (cap_t*)proc->files;
        if (caps && (caps->effective & (1ULL << cap))) {
            return true;
        }
        return false;
    }
    
    /* Non-root: check capability set */
    cap_t* caps = (cap_t*)proc->files;
    if (!caps) return false;
    
    return (caps->effective & (1ULL << cap)) != 0;
}

int cap_set(int cap, bool value) {
    if (cap < 0 || cap > CAP_LAST_CAP) return -1;
    
    process_t* proc = process_current();
    if (!proc) return -1;
    
    cap_t* caps = (cap_t*)proc->files;
    if (!caps) {
        caps = (cap_t*)kzalloc(sizeof(cap_t));
        if (!caps) return -1;
        proc->files = caps;
    }
    
    if (value) {
        caps->effective |= (1ULL << cap);
        caps->permitted |= (1ULL << cap);
    } else {
        caps->effective &= ~(1ULL << cap);
    }
    
    return 0;
}

int cap_get(cap_t* caps) {
    if (!caps) return -1;
    
    process_t* proc = process_current();
    if (!proc) return -1;
    
    cap_t* proc_caps = (cap_t*)proc->files;
    if (!proc_caps) {
        memset(caps, 0, sizeof(cap_t));
        return 0;
    }
    
    *caps = *proc_caps;
    return 0;
}

int cap_set_all(cap_t* caps) {
    if (!caps) return -1;
    
    process_t* proc = process_current();
    if (!proc) return -1;
    
    cap_t* proc_caps = (cap_t*)proc->files;
    if (!proc_caps) {
        proc_caps = (cap_t*)kzalloc(sizeof(cap_t));
        if (!proc_caps) return -1;
        proc->files = proc_caps;
    }
    
    *proc_caps = *caps;
    return 0;
}
