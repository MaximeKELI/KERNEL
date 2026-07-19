#include "wait.h"
#include "scheduler.h"
#include "process.h"
#include "drivers/timer.h"

/*
 * Wait queues and timed sleep for kernel tasks. Blocking removes the task from
 * the runqueue (schedule() does not requeue a task whose state is not RUNNING);
 * waking puts it back via wake_up_process(). On a single CPU we serialize purely
 * by disabling interrupts around the list mutations.
 */

extern process_t* current_process;

/* Tasks sleeping with a deadline, linked through process_t.wait_next. */
static process_t* sleeper_head = NULL;

void wait_queue_init(wait_queue_t* wq) {
    if (wq) {
        wq->head = NULL;
    }
}

void wait_block(wait_queue_t* wq) {
    if (!wq) {
        return;
    }
    u64 flags = local_irq_save();
    process_t* cur = current_process;
    cur->state = PROCESS_BLOCKED;
    cur->wait_next = wq->head;
    wq->head = cur;
    schedule();                 /* returns once we are woken and rescheduled */
    local_irq_restore(flags);
}

static void wq_wake(wait_queue_t* wq, bool all) {
    if (!wq) {
        return;
    }
    u64 flags = local_irq_save();
    process_t* p = wq->head;
    wq->head = NULL;
    while (p) {
        process_t* next = p->wait_next;
        p->wait_next = NULL;
        wake_up_process(p);
        if (!all) {
            wq->head = next;    /* keep the remaining waiters queued */
            break;
        }
        p = next;
    }
    local_irq_restore(flags);
}

void wait_wake_one(wait_queue_t* wq) {
    wq_wake(wq, false);
}

void wait_wake_all(wait_queue_t* wq) {
    wq_wake(wq, true);
}

void sched_sleep(u64 ticks) {
    if (ticks == 0) {
        return;
    }
    u64 flags = local_irq_save();
    process_t* cur = current_process;
    cur->sleep_until = timer_get_ticks() + ticks;
    cur->state = PROCESS_BLOCKED;
    cur->wait_next = sleeper_head;
    sleeper_head = cur;
    schedule();                 /* yields the CPU until the timer wakes us */
    local_irq_restore(flags);
}

void sched_timer_wakeups(void) {
    /* Called from the timer IRQ (interrupts already disabled). */
    u64 now = timer_get_ticks();
    process_t** pp = &sleeper_head;
    while (*pp) {
        process_t* p = *pp;
        if (p->sleep_until <= now) {
            *pp = p->wait_next;
            p->wait_next = NULL;
            wake_up_process(p);
        } else {
            pp = &p->wait_next;
        }
    }
}
