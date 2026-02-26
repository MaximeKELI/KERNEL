#include "seccomp.h"
#include "process.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static seccomp_filter_t* seccomp_filters = NULL;
static spinlock_t seccomp_lock = SPINLOCK_INIT;

/* Allowed syscalls in strict mode */
static const u64 strict_allowed[] = {
    SYS_READ, SYS_WRITE, SYS_EXIT, SYS_SIGRETURN
};

void seccomp_init(void) {
    DEBUG_INFO("Seccomp (secure computing) initialized");
}

int seccomp_set_mode_strict(void) {
    process_t* proc = process_current();
    if (!proc) return -1;
    
    seccomp_filter_t* filter = (seccomp_filter_t*)kzalloc(sizeof(seccomp_filter_t));
    if (!filter) return -1;
    
    filter->mode = SECCOMP_MODE_STRICT;
    
    spinlock_lock(&seccomp_lock);
    filter->next = seccomp_filters;
    seccomp_filters = filter;
    proc->files = filter;
    spinlock_unlock(&seccomp_lock);
    
    DEBUG_INFO("Seccomp strict mode enabled");
    return 0;
}

int seccomp_set_mode_filter(void* filter_data, size_t size) {
    process_t* proc = process_current();
    if (!proc) return -1;
    
    seccomp_filter_t* filter = (seccomp_filter_t*)kzalloc(sizeof(seccomp_filter_t));
    if (!filter) return -1;
    
    filter->mode = SECCOMP_MODE_FILTER;
    filter->filter = kmalloc(size);
    if (!filter->filter) {
        kfree(filter);
        return -1;
    }
    memcpy(filter->filter, filter_data, size);
    filter->filter_size = size;
    
    spinlock_lock(&seccomp_lock);
    filter->next = seccomp_filters;
    seccomp_filters = filter;
    proc->files = filter;
    spinlock_unlock(&seccomp_lock);
    
    DEBUG_INFO("Seccomp filter mode enabled");
    return 0;
}

int seccomp_get_mode(void) {
    process_t* proc = process_current();
    if (!proc) return SECCOMP_MODE_DISABLED;
    
    seccomp_filter_t* filter = (seccomp_filter_t*)proc->files;
    return filter ? filter->mode : SECCOMP_MODE_DISABLED;
}

bool seccomp_check_syscall(u64 syscall_num) {
    process_t* proc = process_current();
    if (!proc) return true;
    
    seccomp_filter_t* filter = (seccomp_filter_t*)proc->files;
    if (!filter) return true;
    
    if (filter->mode == SECCOMP_MODE_STRICT) {
        for (u32 i = 0; i < sizeof(strict_allowed) / sizeof(strict_allowed[0]); i++) {
            if (strict_allowed[i] == syscall_num) {
                return true;
            }
        }
        return false;
    }
    
    /* Filter mode would check BPF filter */
    return true;
}
