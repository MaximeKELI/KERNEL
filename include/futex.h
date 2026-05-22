#ifndef FUTEX_H
#define FUTEX_H

#include "types.h"
#include "spinlock.h"
#include "timerfd.h"

/* Futex operations */
#define FUTEX_WAIT        0
#define FUTEX_WAKE        1
#define FUTEX_FD          2
#define FUTEX_REQUEUE     3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP     5
#define FUTEX_LOCK_PI     6
#define FUTEX_UNLOCK_PI   7
#define FUTEX_TRYLOCK_PI  8
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10

/* Futex flags */
#define FUTEX_PRIVATE_FLAG  128
#define FUTEX_CLOCK_REALTIME 256
#define FUTEX_CMD_MASK      ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

/* Futex wait bitset */
#define FUTEX_BITSET_MATCH_ANY 0xFFFFFFFF

/* Futex word structure */
typedef struct futex_word {
    volatile u32 value;
    spinlock_t lock;
    struct futex_wait_queue* waiters;
    struct futex_word* next;
} futex_word_t;

/* Futex system call */
long futex(u32* uaddr, i32 op, u32 val, const struct timespec* timeout, u32* uaddr2, u32 val3);

/* Fast path futex operations */
int futex_wait(u32* uaddr, u32 val);
int futex_wake(u32* uaddr, i32 nr_wake);
int futex_requeue(u32* uaddr, i32 nr_wake, i32 nr_requeue, u32* uaddr2);

#endif /* FUTEX_H */
