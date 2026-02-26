#include "eventfd.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

#define MAX_EVENTFD_FDS 256

typedef struct eventfd_entry {
    i32 fd;
    u64 counter;
    bool semaphore;
    bool nonblock;
    bool active;
    spinlock_t lock;
} eventfd_entry_t;

static eventfd_entry_t eventfd_table[MAX_EVENTFD_FDS];
static i32 next_fd = 0;
static spinlock_t eventfd_global_lock = SPINLOCK_INIT;

i32 eventfd(u32 initval, i32 flags) {
    spinlock_lock(&eventfd_global_lock);
    
    for (u32 i = 0; i < MAX_EVENTFD_FDS; i++) {
        if (!eventfd_table[i].active) {
            eventfd_entry_t* entry = &eventfd_table[i];
            entry->fd = next_fd++;
            entry->counter = initval;
            entry->semaphore = (flags & EFD_SEMAPHORE) != 0;
            entry->nonblock = (flags & EFD_NONBLOCK) != 0;
            entry->active = true;
            spinlock_init(&entry->lock);
            
            spinlock_unlock(&eventfd_global_lock);
            DEBUG_INFO("Eventfd created: fd=%d, initval=%u, flags=0x%x", entry->fd, initval, flags);
            return entry->fd;
        }
    }
    
    spinlock_unlock(&eventfd_global_lock);
    DEBUG_ERROR("%s", "Maximum eventfd instances reached");
    return -1;
}

ssize_t eventfd_read(i32 fd, u64* value) {
    VALIDATE_PTR_RET(value, -1);
    
    if (fd < 0 || fd >= MAX_EVENTFD_FDS || !eventfd_table[fd].active) {
        return -1;
    }
    
    eventfd_entry_t* entry = &eventfd_table[fd];
    spinlock_lock(&entry->lock);
    
    if (entry->counter == 0) {
        spinlock_unlock(&entry->lock);
        if (entry->nonblock) {
            return -1; /* Would set EAGAIN */
        }
        return 0; /* Block until counter > 0 */
    }
    
    if (entry->semaphore) {
        *value = 1;
        entry->counter--;
    } else {
        *value = entry->counter;
        entry->counter = 0;
    }
    
    spinlock_unlock(&entry->lock);
    return sizeof(u64);
}

ssize_t eventfd_write(i32 fd, u64 value) {
    if (fd < 0 || fd >= MAX_EVENTFD_FDS || !eventfd_table[fd].active) {
        return -1;
    }
    
    if (value == UINT64_MAX) {
        return -1; /* Invalid value */
    }
    
    eventfd_entry_t* entry = &eventfd_table[fd];
    spinlock_lock(&entry->lock);
    
    u64 max = UINT64_MAX - entry->counter;
    if (value > max) {
        spinlock_unlock(&entry->lock);
        if (entry->nonblock) {
            return -1; /* Would set EAGAIN */
        }
        return 0; /* Block until space available */
    }
    
    entry->counter += value;
    spinlock_unlock(&entry->lock);
    return sizeof(u64);
}
