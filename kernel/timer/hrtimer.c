#include "timer_hr.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "drivers/timer.h"

static hrtimer_t* timer_list = NULL;
static spinlock_t timer_lock = SPINLOCK_INIT;
static u64 base_time = 0;

void hrtimer_init(void) {
    base_time = timer_get_ticks() * 10000000; /* Convert to nanoseconds */
    DEBUG_INFO("High-resolution timers initialized");
}

u64 hrtimer_get_time(void) {
    return base_time + (timer_get_ticks() * 10000000);
}

hrtimer_t* hrtimer_add(u64 expires_ns, void (*function)(void*), void* data) {
    hrtimer_t* timer = (hrtimer_t*)kzalloc(sizeof(hrtimer_t));
    if (!timer) {
        DEBUG_ERROR("Failed to allocate timer");
        return NULL;
    }
    
    timer->expires = hrtimer_get_time() + expires_ns;
    timer->function = function;
    timer->data = data;
    
    spinlock_lock(&timer_lock);
    
    /* Insert in sorted order */
    hrtimer_t** prev = &timer_list;
    hrtimer_t* current = timer_list;
    
    while (current && current->expires < timer->expires) {
        prev = &current->next;
        current = current->next;
    }
    
    timer->next = current;
    *prev = timer;
    
    spinlock_unlock(&timer_lock);
    
    return timer;
}

void hrtimer_remove(hrtimer_t* timer) {
    if (!timer) return;
    
    spinlock_lock(&timer_lock);
    
    hrtimer_t** prev = &timer_list;
    hrtimer_t* current = timer_list;
    
    while (current) {
        if (current == timer) {
            *prev = current->next;
            kfree(timer);
            spinlock_unlock(&timer_lock);
            return;
        }
        prev = &current->next;
        current = current->next;
    }
    
    spinlock_unlock(&timer_lock);
}

void hrtimer_process(void) {
    u64 now = hrtimer_get_time();
    
    spinlock_lock(&timer_lock);
    
    while (timer_list && timer_list->expires <= now) {
        hrtimer_t* timer = timer_list;
        timer_list = timer->next;
        
        void (*function)(void*) = timer->function;
        void* data = timer->data;
        
        kfree(timer);
        
        spinlock_unlock(&timer_lock);
        
        if (function) {
            function(data);
        }
        
        spinlock_lock(&timer_lock);
    }
    
    spinlock_unlock(&timer_lock);
}









