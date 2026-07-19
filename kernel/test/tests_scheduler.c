#include "test.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "drivers/timer.h"

/* Test process creation */
static test_result_t test_process_create(void) {
    process_t* proc = process_create((void*)0x1000, 4096);
    TEST_ASSERT_NOT_NULL(proc);
    TEST_ASSERT_NE(proc->pid, 0);
    
    return TEST_PASS;
}

/* Test scheduler stats */
static test_result_t test_scheduler_stats(void) {
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    
    TEST_ASSERT_NE(stats.total_runtime, 0);
    
    return TEST_PASS;
}

/* -------------------------------------------------------------------------- */
/* Preemptive multitasking                                                    */
/* -------------------------------------------------------------------------- */

#define WORKER_STACK_SIZE (16 * 1024)

static volatile u64 worker_counters[3];
static volatile int workers_stop;

/* A pure CPU-bound loop that NEVER yields: only timer preemption can share the
 * CPU between several instances of it. */
static void counter_worker(void* arg) {
    volatile u64* counter = (volatile u64*)arg;
    while (!workers_stop) {
        (*counter)++;
    }
}

/*
 * Proof of real preemptive concurrency: three non-yielding kernel threads must
 * all make progress purely because the timer preempts them and the scheduler
 * rotates the CPU between them.
 */
static test_result_t test_preemptive_concurrency(void) {
    scheduler_stats_t before, after;
    scheduler_get_stats(&before);

    workers_stop = 0;
    worker_counters[0] = worker_counters[1] = worker_counters[2] = 0;

    process_t* t0 = kthread_run(counter_worker, (void*)&worker_counters[0], WORKER_STACK_SIZE);
    process_t* t1 = kthread_run(counter_worker, (void*)&worker_counters[1], WORKER_STACK_SIZE);
    process_t* t2 = kthread_run(counter_worker, (void*)&worker_counters[2], WORKER_STACK_SIZE);
    TEST_ASSERT_NOT_NULL(t0);
    TEST_ASSERT_NOT_NULL(t1);
    TEST_ASSERT_NOT_NULL(t2);

    /* Give the scheduler time to rotate across all of them (~400ms at 100Hz). */
    timer_sleep(40);

    u64 c0 = worker_counters[0];
    u64 c1 = worker_counters[1];
    u64 c2 = worker_counters[2];

    /* Ask the workers to finish and let them exit before returning. */
    workers_stop = 1;
    timer_sleep(10);

    scheduler_get_stats(&after);

    /* Every worker ran => the CPU was preemptively shared between them. */
    TEST_ASSERT_NE(c0, 0);
    TEST_ASSERT_NE(c1, 0);
    TEST_ASSERT_NE(c2, 0);

    /* Real context switches happened while they ran. */
    TEST_ASSERT_NE(after.total_switches, before.total_switches);

    return TEST_PASS;
}

/* -------------------------------------------------------------------------- */
/* Context-switch register integrity                                          */
/* -------------------------------------------------------------------------- */

/* Keeps `sig` live in every callee-saved register across a preemptible loop and
 * returns 1 iff a context switch corrupted them (kernel/asm/context_switch.S). */
extern int reg_integrity_loop(u64 sig, volatile int* stop);

static volatile int integ_stop;
static volatile int integ_fail;
static volatile int integ_done;

static void integ_worker(void* arg) {
    u64 sig = (u64)arg;
    if (reg_integrity_loop(sig, &integ_stop) != 0) {
        __atomic_store_n(&integ_fail, 1, __ATOMIC_SEQ_CST);
    }
    __atomic_add_fetch(&integ_done, 1, __ATOMIC_SEQ_CST);
}

/*
 * Two threads each pin a distinct signature into rbx/r12-r15 and spin. If
 * switch_to() fails to preserve callee-saved registers, one thread will observe
 * the other's signature and flag corruption.
 */
static test_result_t test_context_switch_integrity(void) {
    integ_stop = 0;
    integ_fail = 0;
    integ_done = 0;

    process_t* a = kthread_run(integ_worker, (void*)0xA5A5A5A5A5A5A5A5ULL, WORKER_STACK_SIZE);
    process_t* b = kthread_run(integ_worker, (void*)0x5A5A5A5A5A5A5A5AULL, WORKER_STACK_SIZE);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);

    timer_sleep(30);
    integ_stop = 1;

    for (int i = 0; i < 20 && __atomic_load_n(&integ_done, __ATOMIC_SEQ_CST) < 2; i++) {
        timer_sleep(5);
    }

    TEST_ASSERT_EQ(integ_fail, 0);
    TEST_ASSERT_EQ(__atomic_load_n(&integ_done, __ATOMIC_SEQ_CST), 2);

    return TEST_PASS;
}

/* Register scheduler tests */
void register_scheduler_tests(void) {
    test_register("scheduler", "process_create", test_process_create);
    test_register("scheduler", "scheduler_stats", test_scheduler_stats);
    test_register("scheduler", "preemptive_concurrency", test_preemptive_concurrency);
    test_register("scheduler", "context_switch_integrity", test_context_switch_integrity);
}
