#include "futex.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"
#include "scheduler.h"

#define MAX_FUTEX_HASH_BUCKETS 256
#define FUTEX_HASH_MASK (MAX_FUTEX_HASH_BUCKETS - 1)

typedef struct futex_waiter {
    process_t* process;
    u32 val;
    struct futex_waiter* next;
} futex_waiter_t;

typedef struct futex_bucket {
    futex_word_t* words;
    spinlock_t lock;
} futex_bucket_t;

static futex_bucket_t futex_hash[MAX_FUTEX_HASH_BUCKETS];

static u32 futex_hash_key(u32* uaddr) {
    return ((u64)uaddr >> 3) & FUTEX_HASH_MASK;
}

static futex_word_t* futex_get_word(u32* uaddr) {
    u32 hash = futex_hash_key(uaddr);
    futex_bucket_t* bucket = &futex_hash[hash];
    
    spinlock_lock(&bucket->lock);
    
    futex_word_t* word = bucket->words;
    while (word) {
        if (word->value == (u32)(u64)uaddr) {
            spinlock_unlock(&bucket->lock);
            return word;
        }
        word = (futex_word_t*)((u64)word->next);
    }
    
    /* Create new futex word */
    word = (futex_word_t*)kmalloc(sizeof(futex_word_t));
    if (word) {
        word->value = (u32)(u64)uaddr;
        word->waiters = NULL;
        spinlock_init(&word->lock);
        word->next = (struct futex_word*)bucket->words;
        bucket->words = word;
    }
    
    spinlock_unlock(&bucket->lock);
    return word;
}

void futex_init(void) {
    for (u32 i = 0; i < MAX_FUTEX_HASH_BUCKETS; i++) {
        futex_hash[i].words = NULL;
        spinlock_init(&futex_hash[i].lock);
    }
    DEBUG_INFO("%s", "Futex subsystem initialized");
}

long futex(u32* uaddr, i32 op, u32 val, const struct timespec* timeout, u32* uaddr2, u32 val3) {
    VALIDATE_PTR_RET(uaddr, -1);
    
    futex_word_t* word = futex_get_word(uaddr);
    if (!word) {
        return -1;
    }
    
    switch (op & FUTEX_CMD_MASK) {
        case FUTEX_WAIT:
            spinlock_lock(&word->lock);
            if (*uaddr != val) {
                spinlock_unlock(&word->lock);
                return -1; /* Would set EAGAIN */
            }
            
            /* Add to wait queue */
            futex_waiter_t* waiter = (futex_waiter_t*)kmalloc(sizeof(futex_waiter_t));
            if (waiter) {
                waiter->process = process_current();
                waiter->val = val;
                waiter->next = (struct futex_waiter*)word->waiters;
                word->waiters = waiter;
                
                /* Block process */
                waiter->process->state = PROCESS_BLOCKED;
            }
            spinlock_unlock(&word->lock);
            
            if (waiter) {
                schedule();
            }
            return 0;
            
        case FUTEX_WAKE:
            spinlock_lock(&word->lock);
            i32 woken = 0;
            futex_waiter_t** prev = (futex_waiter_t**)&word->waiters;
            futex_waiter_t* w = (futex_waiter_t*)word->waiters;
            
            while (w && woken < val) {
                w->process->state = PROCESS_READY;
                *prev = w->next;
                kfree(w);
                w = (futex_waiter_t*)*prev;
                woken++;
            }
            spinlock_unlock(&word->lock);
            return woken;
            
        case FUTEX_REQUEUE:
            VALIDATE_PTR_RET(uaddr2, -1);
            return futex_requeue(uaddr, val, val3, uaddr2);
            
        default:
            DEBUG_ERROR("Unsupported futex operation: %d", op);
            return -1;
    }
    
    (void)timeout;
    (void)uaddr2;
    (void)val3;
}

int futex_wait(u32* uaddr, u32 val) {
    return (int)futex(uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}

int futex_wake(u32* uaddr, i32 nr_wake) {
    return (int)futex(uaddr, FUTEX_WAKE, nr_wake, NULL, NULL, 0);
}

int futex_requeue(u32* uaddr, i32 nr_wake, i32 nr_requeue, u32* uaddr2) {
    VALIDATE_PTR_RET(uaddr, -1);
    VALIDATE_PTR_RET(uaddr2, -1);
    
    futex_word_t* word1 = futex_get_word(uaddr);
    futex_word_t* word2 = futex_get_word(uaddr2);
    
    if (!word1 || !word2) {
        return -1;
    }
    
    spinlock_lock(&word1->lock);
    spinlock_lock(&word2->lock);
    
    i32 woken = 0;
    i32 requeued = 0;
    
    /* Wake some waiters */
    futex_waiter_t** prev = (futex_waiter_t**)&word1->waiters;
    futex_waiter_t* w = (futex_waiter_t*)word1->waiters;
    
    while (w && woken < nr_wake) {
        w->process->state = PROCESS_READY;
        *prev = w->next;
        kfree(w);
        w = (futex_waiter_t*)*prev;
        woken++;
    }
    
    /* Requeue remaining */
    while (w && requeued < nr_requeue) {
        futex_waiter_t* next = w->next;
        w->next = (struct futex_waiter*)word2->waiters;
        word2->waiters = w;
        w = (futex_waiter_t*)next;
        requeued++;
    }
    
    word1->waiters = (struct futex_wait_queue*)w;
    
    spinlock_unlock(&word2->lock);
    spinlock_unlock(&word1->lock);
    
    return woken;
}
