#include "sched_deadline.h"
#include "process.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static process_t* deadline_queue = NULL;
static spinlock_t deadline_lock = SPINLOCK_INIT;

void sched_deadline_init(void) {
    DEBUG_INFO("%s", "Deadline scheduler initialized");
}

int sched_setattr_deadline(u64 pid, sched_dl_param_t* params) {
    if (!params) return -1;
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            /* Store deadline parameters (use files field as temporary storage) */
            proc->files = params;
            proc->priority = SCHED_DEADLINE;
            
            /* Add to deadline queue */
            spinlock_lock(&deadline_lock);
            proc->next = deadline_queue;
            deadline_queue = proc;
            spinlock_unlock(&deadline_lock);
            
            DEBUG_INFO("Deadline set for process %u: runtime=%u, deadline=%u, period=%u",
                      (u32)pid, (u32)params->runtime, (u32)params->deadline, (u32)params->period);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int sched_getattr_deadline(u64 pid, sched_dl_param_t* params) {
    if (!params) return -1;
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid && proc->priority == SCHED_DEADLINE) {
            sched_dl_param_t* dl_params = (sched_dl_param_t*)proc->files;
            if (dl_params) {
                *params = *dl_params;
                return 0;
            }
        }
        proc = proc->next;
    }
    
    return -1;
}

void sched_deadline_schedule(void) {
    spinlock_lock(&deadline_lock);
    
    /* Find earliest deadline */
    process_t* earliest = NULL;
    u64 earliest_deadline = UINT64_MAX;
    
    process_t* proc = deadline_queue;
    while (proc) {
        sched_dl_param_t* params = (sched_dl_param_t*)proc->files;
        if (params && params->deadline < earliest_deadline) {
            earliest_deadline = params->deadline;
            earliest = proc;
        }
        proc = proc->next;
    }
    
    if (earliest) {
        earliest->state = PROCESS_RUNNING;
    }
    
    spinlock_unlock(&deadline_lock);
}
