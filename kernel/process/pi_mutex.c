#include "pi_mutex.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "scheduler.h"

void pi_mutex_init(pi_mutex_t* mutex) {
    VALIDATE_PTR_VOID(mutex);
    
    mutex->owner = NULL;
    mutex->priority = 0;
    mutex->original_priority = 0;
    mutex->next = NULL;
    spinlock_init(&mutex->lock);
}

int pi_mutex_lock(pi_mutex_t* mutex) {
    VALIDATE_PTR_RET(mutex, -1);
    
    process_t* current = process_current();
    if (!current) {
        return -1;
    }
    
    spinlock_lock(&mutex->lock);
    
    if (mutex->owner == NULL) {
        /* Lock is free */
        mutex->owner = current;
        mutex->priority = current->priority;
        mutex->original_priority = current->priority;
        spinlock_unlock(&mutex->lock);
        return 0;
    }
    
    if (mutex->owner == current) {
        /* Recursive lock - would need count */
        spinlock_unlock(&mutex->lock);
        return -1; /* Would set EDEADLK */
    }
    
    /* Priority inheritance: boost owner's priority */
    if (current->priority < mutex->owner->priority) {
        pi_mutex_boost_priority(mutex->owner, current->priority);
    }
    
    /* Block current process */
    current->state = PROCESS_BLOCKED;
    spinlock_unlock(&mutex->lock);
    
    schedule();
    
    /* After wakeup, try again */
    return pi_mutex_lock(mutex);
}

int pi_mutex_unlock(pi_mutex_t* mutex) {
    VALIDATE_PTR_RET(mutex, -1);
    
    process_t* current = process_current();
    if (!current) {
        return -1;
    }
    
    spinlock_lock(&mutex->lock);
    
    if (mutex->owner != current) {
        spinlock_unlock(&mutex->lock);
        return -1; /* Would set EPERM */
    }
    
    /* Restore original priority */
    if (mutex->original_priority != mutex->owner->priority) {
        pi_mutex_restore_priority(mutex->owner);
    }
    
    mutex->owner = NULL;
    mutex->priority = 0;
    mutex->original_priority = 0;
    
    /* Would wake waiters here */
    
    spinlock_unlock(&mutex->lock);
    return 0;
}

int pi_mutex_trylock(pi_mutex_t* mutex) {
    VALIDATE_PTR_RET(mutex, -1);
    
    process_t* current = process_current();
    if (!current) {
        return -1;
    }
    
    spinlock_lock(&mutex->lock);
    
    if (mutex->owner == NULL) {
        mutex->owner = current;
        mutex->priority = current->priority;
        mutex->original_priority = current->priority;
        spinlock_unlock(&mutex->lock);
        return 0;
    }
    
    spinlock_unlock(&mutex->lock);
    return -1; /* Would set EBUSY */
}

void pi_mutex_boost_priority(process_t* process, u32 new_priority) {
    if (!process) return;
    
    spinlock_lock(&process->lock);
    if (new_priority < process->priority) {
        process->priority = new_priority;
        DEBUG_INFO("Priority boosted: pid=%u, new_priority=%u", process->pid, new_priority);
    }
    spinlock_unlock(&process->lock);
}

void pi_mutex_restore_priority(process_t* process) {
    if (!process) return;
    
    /* Would restore from saved priority list */
    DEBUG_INFO("Priority restored: pid=%u", process->pid);
}
