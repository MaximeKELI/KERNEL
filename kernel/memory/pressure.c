#include "memory_pressure.h"
#include "memory.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "string.h"
#include "types.h"

/* Memory pressure levels */
#define PRESSURE_LOW     0
#define PRESSURE_MEDIUM  1
#define PRESSURE_HIGH    2
#define PRESSURE_CRITICAL 3

/* Pressure thresholds (percentage of free memory) */
#define PRESSURE_LOW_THRESHOLD      80
#define PRESSURE_MEDIUM_THRESHOLD   60
#define PRESSURE_HIGH_THRESHOLD     40
#define PRESSURE_CRITICAL_THRESHOLD  20

static u32 current_pressure_level = PRESSURE_LOW;
static spinlock_t pressure_lock = SPINLOCK_INIT;
static u64 pressure_stats[4] = {0}; /* Stats per level */

/* Callbacks for pressure events */
typedef void (*pressure_callback_t)(u32 level);
static pressure_callback_t callbacks[16];
static u32 callback_count = 0;

void memory_pressure_init(void) {
    current_pressure_level = PRESSURE_LOW;
    callback_count = 0;
    memset(pressure_stats, 0, sizeof(pressure_stats));
    printk("[Memory Pressure] Initialized\n");
}

u32 memory_pressure_check(void) {
    size_t free_pages = pmm_get_free_pages();
    size_t total_pages = pmm_get_total_pages();
    
    if (total_pages == 0) return PRESSURE_CRITICAL;
    
    u32 free_percent = (free_pages * 100) / total_pages;
    u32 old_level = current_pressure_level;
    
    spinlock_lock(&pressure_lock);
    
    if (free_percent >= PRESSURE_LOW_THRESHOLD) {
        current_pressure_level = PRESSURE_LOW;
    } else if (free_percent >= PRESSURE_MEDIUM_THRESHOLD) {
        current_pressure_level = PRESSURE_MEDIUM;
    } else if (free_percent >= PRESSURE_HIGH_THRESHOLD) {
        current_pressure_level = PRESSURE_HIGH;
    } else {
        current_pressure_level = PRESSURE_CRITICAL;
    }
    
    pressure_stats[current_pressure_level]++;
    
    /* Notify callbacks if level changed */
    if (old_level != current_pressure_level) {
        for (u32 i = 0; i < callback_count; i++) {
            if (callbacks[i]) {
                callbacks[i](current_pressure_level);
            }
        }
    }
    
    spinlock_unlock(&pressure_lock);
    
    return current_pressure_level;
}

u32 memory_pressure_get_level(void) {
    spinlock_lock(&pressure_lock);
    u32 level = current_pressure_level;
    spinlock_unlock(&pressure_lock);
    return level;
}

bool memory_pressure_register_callback(pressure_callback_t callback) {
    if (!callback || callback_count >= 16) {
        return false;
    }
    
    spinlock_lock(&pressure_lock);
    callbacks[callback_count++] = callback;
    spinlock_unlock(&pressure_lock);
    
    return true;
}

void memory_pressure_get_stats(u64 stats[4]) {
    spinlock_lock(&pressure_lock);
    memcpy(stats, pressure_stats, sizeof(pressure_stats));
    spinlock_unlock(&pressure_lock);
}

const char* memory_pressure_level_name(u32 level) {
    switch (level) {
        case PRESSURE_LOW: return "LOW";
        case PRESSURE_MEDIUM: return "MEDIUM";
        case PRESSURE_HIGH: return "HIGH";
        case PRESSURE_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}
