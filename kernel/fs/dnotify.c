#include "dnotify.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

#define MAX_DNOTIFY_FDS 256

typedef struct dnotify_entry {
    i32 fd;
    u32 mask;
    u32 pending_events;
    bool active;
    spinlock_t lock;
} dnotify_entry_t;

static dnotify_entry_t dnotify_table[MAX_DNOTIFY_FDS];
static spinlock_t dnotify_global_lock = SPINLOCK_INIT;

static dnotify_entry_t* dnotify_get_entry(i32 fd) {
    if (fd < 0 || fd >= MAX_DNOTIFY_FDS) {
        return NULL;
    }
    
    if (dnotify_table[fd].active) {
        return &dnotify_table[fd];
    }
    
    return NULL;
}

int dnotify_init(i32 fd, u32 mask) {
    if (fd < 0 || fd >= MAX_DNOTIFY_FDS) {
        DEBUG_ERROR("Invalid fd for dnotify: %d", fd);
        return -1;
    }
    
    spinlock_lock(&dnotify_global_lock);
    
    dnotify_entry_t* entry = &dnotify_table[fd];
    if (entry->active) {
        spinlock_unlock(&dnotify_global_lock);
        DEBUG_WARN("Dnotify already initialized for fd %d", fd);
        return -1;
    }
    
    entry->fd = fd;
    entry->mask = mask;
    entry->pending_events = 0;
    entry->active = true;
    spinlock_init(&entry->lock);
    
    spinlock_unlock(&dnotify_global_lock);
    
    DEBUG_INFO("Dnotify initialized: fd=%d, mask=0x%x", fd, mask);
    return 0;
}

u32 dnotify_get_events(i32 fd) {
    dnotify_entry_t* entry = dnotify_get_entry(fd);
    if (!entry) {
        return 0;
    }
    
    spinlock_lock(&entry->lock);
    u32 events = entry->pending_events;
    spinlock_unlock(&entry->lock);
    
    return events;
}

void dnotify_clear_events(i32 fd, u32 mask) {
    dnotify_entry_t* entry = dnotify_get_entry(fd);
    if (!entry) {
        return;
    }
    
    spinlock_lock(&entry->lock);
    entry->pending_events &= ~mask;
    spinlock_unlock(&entry->lock);
}

/* Internal function to signal event */
void dnotify_signal_event(i32 fd, u32 event) {
    dnotify_entry_t* entry = dnotify_get_entry(fd);
    if (!entry) {
        return;
    }
    
    spinlock_lock(&entry->lock);
    if (entry->mask & event) {
        entry->pending_events |= event;
    }
    spinlock_unlock(&entry->lock);
}
