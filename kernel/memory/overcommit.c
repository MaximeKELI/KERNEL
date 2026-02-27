#include "memory_overcommit.h"
#include "memory.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Overcommit modes */
#define OVERCOMMIT_NONE    0
#define OVERCOMMIT_ALWAYS  1
#define OVERCOMMIT_GUESS   2

static u32 overcommit_mode = OVERCOMMIT_GUESS;
static u64 overcommit_ratio = 50; /* 50% overcommit allowed */
static spinlock_t overcommit_lock = SPINLOCK_INIT;
static u64 overcommit_allocations = 0;
static u64 overcommit_failures = 0;

void memory_overcommit_init(void) {
    overcommit_mode = OVERCOMMIT_GUESS;
    overcommit_ratio = 50;
    overcommit_allocations = 0;
    overcommit_failures = 0;
    printk("[Memory Overcommit] Initialized (mode: GUESS, ratio: %u%%)\n", (u32)overcommit_ratio);
}

bool memory_overcommit_allowed(size_t requested_pages) {
    spinlock_lock(&overcommit_lock);
    
    bool allowed = false;
    
    switch (overcommit_mode) {
        case OVERCOMMIT_NONE:
            /* No overcommit - check if we have enough free */
            {
                size_t free_pages = pmm_get_free_pages();
                allowed = (free_pages >= requested_pages);
            }
            break;
            
        case OVERCOMMIT_ALWAYS:
            /* Always allow overcommit */
            allowed = true;
            break;
            
        case OVERCOMMIT_GUESS:
            /* Heuristic: allow if free + (total * ratio / 100) >= requested */
            {
                size_t free_pages = pmm_get_free_pages();
                size_t total_pages = pmm_get_total_pages();
                size_t overcommit_limit = (total_pages * overcommit_ratio) / 100;
                allowed = ((free_pages + overcommit_limit) >= requested_pages);
            }
            break;
    }
    
    if (allowed) {
        overcommit_allocations++;
    } else {
        overcommit_failures++;
    }
    
    spinlock_unlock(&overcommit_lock);
    
    return allowed;
}

void memory_overcommit_set_mode(u32 mode) {
    if (mode > OVERCOMMIT_GUESS) {
        return;
    }
    
    spinlock_lock(&overcommit_lock);
    overcommit_mode = mode;
    spinlock_unlock(&overcommit_lock);
    
    printk("[Memory Overcommit] Mode set to %u\n", mode);
}

u32 memory_overcommit_get_mode(void) {
    spinlock_lock(&overcommit_lock);
    u32 mode = overcommit_mode;
    spinlock_unlock(&overcommit_lock);
    return mode;
}

void memory_overcommit_set_ratio(u64 ratio) {
    if (ratio > 100) {
        ratio = 100;
    }
    
    spinlock_lock(&overcommit_lock);
    overcommit_ratio = ratio;
    spinlock_unlock(&overcommit_lock);
    
    printk("[Memory Overcommit] Ratio set to %u%%\n", (u32)ratio);
}

u64 memory_overcommit_get_ratio(void) {
    spinlock_lock(&overcommit_lock);
    u64 ratio = overcommit_ratio;
    spinlock_unlock(&overcommit_lock);
    return ratio;
}

void memory_overcommit_get_stats(u64* allocations, u64* failures) {
    spinlock_lock(&overcommit_lock);
    if (allocations) *allocations = overcommit_allocations;
    if (failures) *failures = overcommit_failures;
    spinlock_unlock(&overcommit_lock);
}
