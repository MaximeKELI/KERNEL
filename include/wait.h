#ifndef WAIT_H
#define WAIT_H

#include "types.h"
#include "process.h"

/*
 * Wait queues and sleeping for kernel tasks.
 *
 * A task blocks by adding itself to a wait_queue_t and calling schedule(); it is
 * made runnable again when another context calls wait_wake_one/all(). Timed
 * sleeps (sched_sleep) block on an internal sleeper list that the timer IRQ
 * drains via sched_timer_wakeups(). All of these are safe to call with
 * interrupts enabled; they serialize on the local IRQ flag (single CPU).
 */

/* Initialize a dynamically-allocated wait queue. */
void wait_queue_init(wait_queue_t* wq);

/*
 * Block the current task on `wq` until woken. Must be called with a condition
 * re-checked by the caller in a loop to guard against spurious wakeups.
 */
void wait_block(wait_queue_t* wq);

/* Wake the first / all tasks blocked on `wq` (no-op if empty). */
void wait_wake_one(wait_queue_t* wq);
void wait_wake_all(wait_queue_t* wq);

/* Block the current task for approximately `ticks` PIT ticks (truly yields). */
void sched_sleep(u64 ticks);

/* Drain expired sleepers; called from the timer IRQ. */
void sched_timer_wakeups(void);

#endif /* WAIT_H */
