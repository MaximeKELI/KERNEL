#include "ai_optimizer.h"
#include "ai_monitor.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "cache.h"
#include "drivers/timer.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "string.h"

static bool optimizer_initialized = false;
static u64 last_memory_cleanup = 0;
static u64 last_priority_adjust = 0;

/* External references */
extern process_t* process_list;

/* Thresholds */
#define CPU_HIGH_THRESHOLD 80
#define MEMORY_PRESSURE_THRESHOLD 85
#define ANOMALY_DETECTION_TICKS 100

void ai_optimizer_init(void) {
    optimizer_initialized = true;
    DEBUG_INFO("AI Optimizer initialized");
}

void ai_optimize_scheduler(void) {
    if (!optimizer_initialized) return;
    
    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    
    /* If CPU usage > 80% → reduce scheduler timeslice */
    if (metrics.cpu_usage > CPU_HIGH_THRESHOLD) {
        printk("[AI] CPU load high (%u%%) → adjusting scheduler\n", (u32)metrics.cpu_usage);
        
        /* Reduce timeslice for all processes */
        process_t* proc = process_list;
        while (proc) {
            if (proc->time_slice > 10) {
                proc->time_slice = proc->time_slice * 3 / 4; /* Reduce by 25% */
            }
            proc = proc->next;
        }
    }
    
    /* If many I/O waits → boost I/O-bound processes */
    if (metrics.context_switches > 50) {
        /* Boost processes that are blocked (likely I/O bound) */
        process_t* proc = process_list;
        while (proc) {
            if (proc->state == PROCESS_BLOCKED) {
                /* Increase priority slightly */
                if (proc->priority < PRIO_MAX) {
                    proc->priority++;
                }
            }
            proc = proc->next;
        }
    }
    
    /* If system idle → increase timeslice for efficiency */
    if (metrics.cpu_usage < 20 && metrics.process_count > 0) {
        process_t* proc = process_list;
        while (proc) {
            if (proc->time_slice < 200) {
                proc->time_slice = proc->time_slice * 5 / 4; /* Increase by 25% */
            }
            proc = proc->next;
        }
    }
}

void ai_optimize_memory(void) {
    if (!optimizer_initialized) return;
    
    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    
    /* If memory pressure detected → trigger memory cleanup */
    if (metrics.memory_usage > MEMORY_PRESSURE_THRESHOLD) {
        u64 current_tick = timer_get_ticks();
        if (current_tick - last_memory_cleanup > 1000) { /* Throttle cleanup */
            printk("[AI] Memory pressure detected (%u%%) → optimizing allocation\n", 
                   (u32)metrics.memory_usage);
            
            /* Trigger cache sync */
            cache_sync_all();
            
            /* Would trigger memory compaction here */
            last_memory_cleanup = current_tick;
        }
    }
}

void ai_detect_anomalies(void) {
    if (!optimizer_initialized) return;
    
    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    
    /* If a process consumes abnormal CPU for too long → reduce its priority */
    static u64 high_cpu_process_ticks = 0;
    
    if (metrics.cpu_usage > CPU_HIGH_THRESHOLD) {
        high_cpu_process_ticks++;
        
        if (high_cpu_process_ticks > ANOMALY_DETECTION_TICKS) {
            printk("[AI] Anomaly detected → lowering process priority\n");
            
            /* Find process with highest runtime and reduce priority */
            process_t* proc = process_list;
            process_t* max_proc = NULL;
            u64 max_runtime = 0;
            
            while (proc) {
                if (proc->runtime > max_runtime && proc->state == PROCESS_RUNNING) {
                    max_runtime = proc->runtime;
                    max_proc = proc;
                }
                proc = proc->next;
            }
            
            if (max_proc && max_proc->priority < PRIO_MAX) {
                max_proc->priority++;
                printk("[AI] Process %u priority reduced (runtime: %u)\n", 
                       (u32)max_proc->pid, (u32)max_proc->runtime);
            }
            
            high_cpu_process_ticks = 0;
        }
    } else {
        high_cpu_process_ticks = 0;
    }
    
    /* Detect abnormal interrupt rate */
    if (metrics.interrupt_rate > 1000) {
        printk("[AI] High interrupt rate detected (%u) → investigating\n", 
               (u32)metrics.interrupt_rate);
    }
}
