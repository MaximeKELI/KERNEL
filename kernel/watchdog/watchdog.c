#include "watchdog.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "drivers/timer.h"

static watchdog_t* watchdog_list = NULL;
static watchdog_t* active_watchdog = NULL;
static u64 last_kick_time = 0;
static spinlock_t watchdog_lock = SPINLOCK_INIT;

void watchdog_init(void) {
    DEBUG_INFO("Watchdog system initialized");
}

int watchdog_register(watchdog_t* wd) {
    if (!wd) return -1;
    
    spinlock_lock(&watchdog_lock);
    wd->next = watchdog_list;
    watchdog_list = wd;
    spinlock_unlock(&watchdog_lock);
    
    DEBUG_INFO("Watchdog registered: %s", wd->name);
    return 0;
}

int watchdog_start(u32 timeout) {
    spinlock_lock(&watchdog_lock);
    
    if (!watchdog_list) {
        spinlock_unlock(&watchdog_lock);
        return -1;
    }
    
    active_watchdog = watchdog_list;
    active_watchdog->timeout = timeout;
    active_watchdog->running = true;
    last_kick_time = timer_get_ticks();
    
    spinlock_unlock(&watchdog_lock);
    
    DEBUG_INFO("Watchdog started: timeout=%u ms", timeout);
    return 0;
}

int watchdog_stop(void) {
    spinlock_lock(&watchdog_lock);
    if (active_watchdog) {
        active_watchdog->running = false;
        active_watchdog = NULL;
    }
    spinlock_unlock(&watchdog_lock);
    
    DEBUG_INFO("Watchdog stopped");
    return 0;
}

void watchdog_kick(void) {
    spinlock_lock(&watchdog_lock);
    
    if (active_watchdog && active_watchdog->running) {
        if (active_watchdog->kick) {
            active_watchdog->kick();
        }
        last_kick_time = timer_get_ticks();
    }
    
    spinlock_unlock(&watchdog_lock);
}
