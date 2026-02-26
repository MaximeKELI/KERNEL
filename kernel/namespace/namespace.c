#include "namespace.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "process.h"
#include "spinlock.h"

static namespace_t* namespace_list = NULL;
static spinlock_t namespace_lock = SPINLOCK_INIT;
static u64 next_ns_id = 1;

void namespace_init(void) {
    /* Create initial namespaces */
    namespace_t* ns = namespace_create(CLONE_NEWPID);
    if (ns) {
        namespace_set(CLONE_NEWPID, ns);
    }
    
    DEBUG_INFO("Namespace system initialized");
}

namespace_t* namespace_create(u64 type) {
    namespace_t* ns = (namespace_t*)kzalloc(sizeof(namespace_t));
    if (!ns) {
        DEBUG_ERROR("Failed to allocate namespace");
        return NULL;
    }
    
    ns->type = type;
    ns->id = next_ns_id++;
    
    spinlock_lock(&namespace_lock);
    ns->next = namespace_list;
    namespace_list = ns;
    spinlock_unlock(&namespace_lock);
    
    DEBUG_INFO("Namespace created: type=0x%x, id=%u", (u32)type, (u32)ns->id);
    return ns;
}

namespace_t* namespace_get(u64 type) {
    process_t* proc = process_current();
    if (!proc) return NULL;
    
    namespace_t* ns = (namespace_t*)proc->private_data;
    while (ns) {
        if (ns->type == type) {
            return ns;
        }
        ns = ns->next;
    }
    
    return NULL;
}

int namespace_set(u64 type, namespace_t* ns) {
    process_t* proc = process_current();
    if (!proc) return -1;
    
    namespace_t* current = (namespace_t*)proc->private_data;
    namespace_t* prev = NULL;
    
    while (current) {
        if (current->type == type) {
            if (prev) {
                prev->next = ns;
            } else {
                proc->private_data = ns;
            }
            ns->next = current->next;
            kfree(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    /* Add new namespace */
    ns->next = (namespace_t*)proc->private_data;
    proc->private_data = ns;
    
    return 0;
}

int unshare(int flags) {
    namespace_t* ns = namespace_create(flags);
    if (!ns) return -1;
    
    return namespace_set(flags, ns);
}

int setns(int fd, int nstype) {
    (void)fd;
    (void)nstype;
    return 0;
}
