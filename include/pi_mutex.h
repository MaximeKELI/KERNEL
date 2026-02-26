#ifndef PI_MUTEX_H
#define PI_MUTEX_H

#include "types.h"
#include "process.h"

/* Priority inheritance mutex */
typedef struct pi_mutex {
    process_t* owner;
    u32 priority;
    u32 original_priority;
    struct pi_mutex* next;
    spinlock_t lock;
} pi_mutex_t;

/* Initialize PI mutex */
void pi_mutex_init(pi_mutex_t* mutex);

/* Lock with priority inheritance */
int pi_mutex_lock(pi_mutex_t* mutex);

/* Unlock with priority inheritance */
int pi_mutex_unlock(pi_mutex_t* mutex);

/* Try lock */
int pi_mutex_trylock(pi_mutex_t* mutex);

/* Boost priority for inheritance */
void pi_mutex_boost_priority(process_t* process, u32 new_priority);

/* Restore original priority */
void pi_mutex_restore_priority(process_t* process);

#endif /* PI_MUTEX_H */
