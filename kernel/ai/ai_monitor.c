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
static u64 last_tick_time = 0;
static spinlock_t metrics_lock = SPINLOCK_INIT;

/* External references */
extern process_t* process_list;

void ai_monitor_init(void) {
    memset(&current_metrics, 0, sizeof(ai_metrics_t));
    DEBUG_INFO("AI Monitor initialized");
}

/* Safe metrics update function - called from timer interrupt */
void ai_update_metrics(void) {
    /* Fast path: check if initialized without lock */
    if (tick_count == 0 && last_tick_time == 0) {
        /* First call - initialize */
        last_tick_time = timer_get_ticks();
    }
    
    spinlock_lock(&metrics_lock);
    
    tick_count++;
    u64 current_tick = timer_get_ticks();
    
    /* Update process count */
    u64 proc_count = 0;
    process_t* proc = process_list;
    while (proc) {
        if (proc->state != PROCESS_DEAD && proc->state != PROCESS_ZOMBIE) {
            proc_count++;
        }
        proc = proc->next;
    }
    current_metrics.process_count = proc_count;
    
    /* Update memory usage percentage */
    size_t total_pages = pmm_get_total_pages();
    size_t free_pages = pmm_get_free_pages();
    if (total_pages > 0) {
        size_t used_pages = total_pages - free_pages;
        current_metrics.memory_usage = (used_pages * 100) / total_pages;
    } else {
        current_metrics.memory_usage = 0;
    }
    
    /* Update context switch rate */
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    if (stats.total_switches > last_context_switches) {
        u64 switches_delta = stats.total_switches - last_context_switches;
        u64 ticks_delta = (current_tick > last_tick_time) ? (current_tick - last_tick_time) : 1;
        /* Context switches per second (assuming 100Hz timer) */
        current_metrics.context_switches = (switches_delta * 100) / ticks_delta;
        last_context_switches = stats.total_switches;
    } else {
        current_metrics.context_switches = 0;
    }
    
    /* Update CPU usage percentage */
    if (stats.total_runtime > 0) {
        u64 cpu_used = stats.total_runtime - stats.idle_time;
        current_metrics.cpu_usage = (cpu_used * 100) / stats.total_runtime;
    } else {
        /* Fallback: estimate from process activity */
        if (proc_count > 0) {
            /* Estimate CPU usage based on active processes */
            u64 active_procs = 0;
            proc = process_list;
            while (proc) {
                if (proc->state == PROCESS_RUNNING || proc->state == PROCESS_READY) {
                    active_procs++;
                }
                proc = proc->next;
            }
            current_metrics.cpu_usage = (active_procs * 100) / (proc_count > 0 ? proc_count : 1);
            if (current_metrics.cpu_usage > 100) current_metrics.cpu_usage = 100;
        } else {
            current_metrics.cpu_usage = 0;
        }
    }
    
    /* Update interrupt frequency */
    /* Track interrupt count from interrupt handler */
    extern u64 global_interrupt_count;
    if (global_interrupt_count > last_interrupt_count) {
        u64 interrupts_delta = global_interrupt_count - last_interrupt_count;
        u64 ticks_delta = (current_tick > last_tick_time) ? (current_tick - last_tick_time) : 1;
        /* Interrupts per second (assuming 100Hz timer) */
        current_metrics.interrupt_rate = (interrupts_delta * 100) / ticks_delta;
        last_interrupt_count = global_interrupt_count;
    } else {
        current_metrics.interrupt_rate = 0;
    }
    
    last_tick_time = current_tick;
    spinlock_unlock(&metrics_lock);
}

/* Alias for backward compatibility */
void ai_monitor_update(void) {
    ai_update_metrics();
}

void ai_monitor_get_metrics(ai_metrics_t* metrics) {
    if (!metrics) return;
    
    spinlock_lock(&metrics_lock);
    *metrics = current_metrics;
    spinlock_unlock(&metrics_lock);
}
