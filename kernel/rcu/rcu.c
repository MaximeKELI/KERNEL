#include "rcu.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "smp.h"

static u64 rcu_gp_ctr = 1;
static spinlock_t rcu_lock = SPINLOCK_INIT;

typedef struct rcu_cb_entry {
    void* ptr;
    rcu_callback_t func;
    struct rcu_cb_entry* next;
} rcu_cb_entry_t;

static rcu_cb_entry_t* callback_list = NULL;

void rcu_init(void) {
    DEBUG_INFO("RCU system initialized");
}

void rcu_read_lock_impl(void) {
    /* Would disable preemption */
    (void)0;
}

void rcu_read_unlock_impl(void) {
    /* Would enable preemption */
    (void)0;
}

void synchronize_rcu(void) {
    spinlock_lock(&rcu_lock);
    rcu_gp_ctr++;
    
    /* Wait for all readers to complete */
    /* Would wait for grace period */
    
    /* Process callbacks */
    rcu_cb_entry_t* cb = callback_list;
    callback_list = NULL;
    spinlock_unlock(&rcu_lock);
    
    while (cb) {
        rcu_cb_entry_t* next = cb->next;
        if (cb->func) {
            cb->func(cb->ptr);
        }
        kfree(cb);
        cb = next;
    }
}

void call_rcu(void* ptr, rcu_callback_t func) {
    rcu_cb_entry_t* cb = (rcu_cb_entry_t*)kmalloc(sizeof(rcu_cb_entry_t));
    if (!cb) return;
    
    cb->ptr = ptr;
    cb->func = func;
    
    spinlock_lock(&rcu_lock);
    cb->next = callback_list;
    callback_list = cb;
    spinlock_unlock(&rcu_lock);
}
