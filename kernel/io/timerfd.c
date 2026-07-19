#include "timerfd.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "drivers/timer.h"

#define MAX_TIMERFD_FDS 256

typedef struct timerfd_entry {
    i32 fd;
    i32 clockid;
    bool nonblock;
    bool active;
    itimerspec_t timer;
    u64 expirations;
    u64 last_expiration;
    spinlock_t lock;
} timerfd_entry_t;

static timerfd_entry_t timerfd_table[MAX_TIMERFD_FDS];
static i32 next_fd = 0;
static spinlock_t timerfd_global_lock = SPINLOCK_INIT;

static void timerfd_tick_callback(u32 timer_id, void* data) {
    timerfd_entry_t* entry = (timerfd_entry_t*)data;
    if (!entry || !entry->active) return;
    
    spinlock_lock(&entry->lock);
    entry->expirations++;
    spinlock_unlock(&entry->lock);
}

i32 timerfd_create(i32 clockid, i32 flags) {
    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC) {
        DEBUG_ERROR("Invalid clockid: %d", clockid);
        return -1;
    }
    
    spinlock_lock(&timerfd_global_lock);
    
    for (u32 i = 0; i < MAX_TIMERFD_FDS; i++) {
        if (!timerfd_table[i].active) {
            timerfd_entry_t* entry = &timerfd_table[i];
            entry->fd = next_fd++;
            entry->clockid = clockid;
            entry->nonblock = (flags & TFD_NONBLOCK) != 0;
            entry->active = true;
            entry->expirations = 0;
            entry->last_expiration = 0;
            memset(&entry->timer, 0, sizeof(entry->timer));
            spinlock_init(&entry->lock);
            
            spinlock_unlock(&timerfd_global_lock);
            DEBUG_INFO("Timerfd created: fd=%d, clockid=%d", entry->fd, clockid);
            return entry->fd;
        }
    }
    
    spinlock_unlock(&timerfd_global_lock);
    DEBUG_ERROR("%s", "Maximum timerfd instances reached");
    return -1;
}

int timerfd_settime(i32 fd, i32 flags, const itimerspec_t* new_value, itimerspec_t* old_value) {
    VALIDATE_PTR_RET(new_value, -1);
    
    if (fd < 0 || fd >= MAX_TIMERFD_FDS || !timerfd_table[fd].active) {
        return -1;
    }
    
    timerfd_entry_t* entry = &timerfd_table[fd];
    spinlock_lock(&entry->lock);
    
    if (old_value) {
        *old_value = entry->timer;
    }
    
    entry->timer = *new_value;
    entry->expirations = 0;
    entry->last_expiration = 0;
    
    /* Would register timer callback here */
    /* timer_register_callback(timerfd_tick_callback, entry); */
    
    spinlock_unlock(&entry->lock);
    
    DEBUG_INFO("Timerfd settime: fd=%d", fd);
    return 0;
}

int timerfd_gettime(i32 fd, itimerspec_t* curr_value) {
    VALIDATE_PTR_RET(curr_value, -1);
    
    if (fd < 0 || fd >= MAX_TIMERFD_FDS || !timerfd_table[fd].active) {
        return -1;
    }
    
    timerfd_entry_t* entry = &timerfd_table[fd];
    spinlock_lock(&entry->lock);
    *curr_value = entry->timer;
    spinlock_unlock(&entry->lock);
    
    return 0;
}

ssize_t timerfd_read(i32 fd, u64* expirations) {
    VALIDATE_PTR_RET(expirations, -1);
    
    if (fd < 0 || fd >= MAX_TIMERFD_FDS || !timerfd_table[fd].active) {
        return -1;
    }
    
    timerfd_entry_t* entry = &timerfd_table[fd];
    spinlock_lock(&entry->lock);
    
    if (entry->expirations == entry->last_expiration) {
        spinlock_unlock(&entry->lock);
        if (entry->nonblock) {
            return -1; /* Would set EAGAIN */
        }
        return 0; /* Block until expiration */
    }
    
    *expirations = entry->expirations - entry->last_expiration;
    entry->last_expiration = entry->expirations;
    
    spinlock_unlock(&entry->lock);
    return sizeof(u64);
}
