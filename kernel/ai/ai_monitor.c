#include "ai_monitor.h"
#include "process.h"
#include "memory.h"
#include "scheduler.h"
#include "interrupt.h"
#include "drivers/timer.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "string.h"

static ai_metrics_t current_metrics = {0};
static u64 last_context_switches = 0;
static u64 last_interrupt_count = 0;
static u64 tick_count = 0;
static spinlock_t metrics_lock = SPINLOCK_INIT;

/* External references */
extern process_t* process_list;
extern scheduler_stats_t scheduler_stats;

void ai_monitor_init(void) {
    memset(&current_metrics, 0, sizeof(ai_metrics_t));
    DEBUG_INFO("AI Monitor initialized");
}

void ai_monitor_update(void) {
    spinlock_lock(&metrics_lock);
    
    tick_count++;
    
    /* Update process count */
    u64 proc_count = 0;
    process_t* proc = process_list;
    while (proc) {
        if (proc->state != PROCESS_DEAD) {
            proc_count++;
        }
        proc = proc->next;
    }
    current_metrics.process_count = proc_count;
    
    /* Update memory usage */
    size_t total_pages = pmm_get_total_pages();
    size_t free_pages = pmm_get_free_pages();
    size_t used_pages = total_pages - free_pages;
    current_metrics.memory_usage = (used_pages * 100) / (total_pages > 0 ? total_pages : 1);
    
    /* Update context switches */
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    if (stats.total_switches > last_context_switches) {
        current_metrics.context_switches = stats.total_switches - last_context_switches;
        last_context_switches = stats.total_switches;
    } else {
        current_metrics.context_switches = 0;
    }
    
    /* Update CPU usage (simplified: based on idle time) */
    if (stats.total_runtime > 0) {
        u64 cpu_used = stats.total_runtime - stats.idle_time;
        current_metrics.cpu_usage = (cpu_used * 100) / stats.total_runtime;
    } else {
        current_metrics.cpu_usage = 0;
    }
    
    /* Update interrupt rate (simplified) */
    current_metrics.interrupt_rate = current_metrics.context_switches; /* Approximation */
    
    spinlock_unlock(&metrics_lock);
}

void ai_monitor_get_metrics(ai_metrics_t* metrics) {
    if (!metrics) return;
    
    spinlock_lock(&metrics_lock);
    *metrics = current_metrics;
    spinlock_unlock(&metrics_lock);
}
