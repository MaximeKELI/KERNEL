#ifndef RCU_H
#define RCU_H

#include "types.h"

/* RCU read-side critical section */
#define rcu_read_lock() do { \
    extern void rcu_read_lock_impl(void); \
    rcu_read_lock_impl(); \
} while(0)

#define rcu_read_unlock() do { \
    extern void rcu_read_unlock_impl(void); \
    rcu_read_unlock_impl(); \
} while(0)

/* RCU synchronization */
void synchronize_rcu(void);

/* RCU callback */
typedef void (*rcu_callback_t)(void*);

/* Call RCU */
void call_rcu(void* ptr, rcu_callback_t func);

/* Initialize RCU */
void rcu_init(void);

/* Per-CPU RCU data */
typedef struct rcu_data {
    u64 ctr;
    u64 gp_ctr;
    void* callbacks;
} rcu_data_t;

#endif /* RCU_H */
