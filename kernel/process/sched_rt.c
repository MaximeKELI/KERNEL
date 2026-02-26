#include "sched_rt.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"
#include "scheduler.h"

#define MAX_RT_PRIORITY 99

static process_t* rt_fifo_queue[MAX_RT_PRIORITY + 1] = {NULL};
static process_t* rt_rr_queue[MAX_RT_PRIORITY + 1] = {NULL};
static spinlock_t rt_sched_lock = SPINLOCK_INIT;

static void rt_enqueue(process_t* proc, i32 policy) {
    if (!proc || proc->priority > MAX_RT_PRIORITY) {
        return;
    }
    
    u32 idx = (u32)proc->priority;
    
    if (policy == SCHED_FIFO) {
        /* Add to FIFO queue (tail) */
        process_t* last = rt_fifo_queue[idx];
        if (!last) {
            rt_fifo_queue[idx] = proc;
            proc->next = NULL;
        } else {
            while (last->next) {
                last = last->next;
            }
            last->next = proc;
            proc->next = NULL;
        }
    } else if (policy == SCHED_RR) {
        /* Add to RR queue (tail) */
        process_t* last = rt_rr_queue[idx];
        if (!last) {
            rt_rr_queue[idx] = proc;
            proc->next = NULL;
        } else {
            while (last->next) {
                last = last->next;
            }
            last->next = proc;
            proc->next = NULL;
        }
    }
}

static void rt_dequeue(process_t* proc, i32 policy) {
    if (!proc) return;
    
    u32 idx = (u32)proc->priority;
    
    if (policy == SCHED_FIFO) {
        if (rt_fifo_queue[idx] == proc) {
            rt_fifo_queue[idx] = proc->next;
            proc->next = NULL;
        } else {
            process_t* p = rt_fifo_queue[idx];
            while (p && p->next != proc) {
                p = p->next;
            }
            if (p) {
                p->next = proc->next;
                proc->next = NULL;
            }
        }
    } else if (policy == SCHED_RR) {
        if (rt_rr_queue[idx] == proc) {
            rt_rr_queue[idx] = proc->next;
            proc->next = NULL;
        } else {
            process_t* p = rt_rr_queue[idx];
            while (p && p->next != proc) {
                p = p->next;
            }
            if (p) {
                p->next = proc->next;
                proc->next = NULL;
            }
        }
    }
}

static process_t* rt_select_next(i32 policy) {
    /* Select highest priority RT process */
    for (i32 prio = MAX_RT_PRIORITY; prio >= RT_PRIO_MIN; prio--) {
        process_t* proc = NULL;
        
        if (policy == SCHED_FIFO || policy == 0) {
            proc = rt_fifo_queue[prio];
        }
        if ((policy == SCHED_RR || policy == 0) && !proc) {
            proc = rt_rr_queue[prio];
        }
        
        if (proc && proc->state == PROCESS_READY) {
            return proc;
        }
    }
    
    return NULL;
}

int sched_setscheduler(u64 pid, i32 policy, const sched_param_t* param) {
    VALIDATE_PTR_RET(param, -1);
    
    if (policy != SCHED_FIFO && policy != SCHED_RR && 
        policy != SCHED_NORMAL && policy != SCHED_BATCH && 
        policy != SCHED_IDLE && policy != SCHED_DEADLINE) {
        DEBUG_ERROR("Invalid scheduling policy: %d", policy);
        return -1;
    }
    
    if ((policy == SCHED_FIFO || policy == SCHED_RR) && 
        (param->sched_priority < RT_PRIO_MIN || param->sched_priority > RT_PRIO_MAX)) {
        DEBUG_ERROR("Invalid RT priority: %d", param->sched_priority);
        return -1;
    }
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            spinlock_lock(&rt_sched_lock);
            
            /* Remove from old queue */
            i32 old_policy = (proc->priority >= RT_PRIO_MIN && proc->priority <= RT_PRIO_MAX) ? 
                            (proc->priority < 50 ? SCHED_FIFO : SCHED_RR) : SCHED_NORMAL;
            rt_dequeue(proc, old_policy);
            
            /* Set new policy and priority */
            proc->priority = param->sched_priority;
            
            /* Add to new queue */
            if (policy == SCHED_FIFO || policy == SCHED_RR) {
                rt_enqueue(proc, policy);
            }
            
            spinlock_unlock(&rt_sched_lock);
            
            DEBUG_INFO("Scheduling policy set: pid=%u, policy=%d, priority=%d", 
                      pid, policy, param->sched_priority);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int sched_getscheduler(u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            if (proc->priority >= RT_PRIO_MIN && proc->priority <= RT_PRIO_MAX) {
                return proc->priority < 50 ? SCHED_FIFO : SCHED_RR;
            }
            return SCHED_NORMAL;
        }
        proc = proc->next;
    }
    
    return -1;
}

int sched_setparam(u64 pid, const sched_param_t* param) {
    VALIDATE_PTR_RET(param, -1);
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            spinlock_lock(&rt_sched_lock);
            proc->priority = param->sched_priority;
            spinlock_unlock(&rt_sched_lock);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int sched_getparam(u64 pid, sched_param_t* param) {
    VALIDATE_PTR_RET(param, -1);
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            param->sched_priority = (i32)proc->priority;
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int sched_yield(void) {
    process_t* current = process_current();
    if (!current) {
        return -1;
    }
    
    spinlock_lock(&rt_sched_lock);
    
    /* For SCHED_RR, move to end of queue */
    if (current->priority >= RT_PRIO_MIN && current->priority <= RT_PRIO_MAX) {
        rt_dequeue(current, SCHED_RR);
        rt_enqueue(current, SCHED_RR);
    }
    
    spinlock_unlock(&rt_sched_lock);
    
    yield();
    return 0;
}

int sched_get_priority_min(i32 policy) {
    if (policy == SCHED_FIFO || policy == SCHED_RR) {
        return RT_PRIO_MIN;
    }
    return 0;
}

int sched_get_priority_max(i32 policy) {
    if (policy == SCHED_FIFO || policy == SCHED_RR) {
        return RT_PRIO_MAX;
    }
    return 0;
}

/* Real-time scheduler entry point */
process_t* sched_rt_select(void) {
    spinlock_lock(&rt_sched_lock);
    process_t* next = rt_select_next(0);
    spinlock_unlock(&rt_sched_lock);
    return next;
}
