#include "test.h"
#include "process.h"
#include "scheduler.h"
#include "wait.h"
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

/* -------------------------------------------------------------------------- */
/* Task lifecycle: join + exit status, blocking sleep, zombie reaping          */
/* -------------------------------------------------------------------------- */

/* Child computes 1+2+...+n and exits with that sum as its status code. */
static void adder_child(void* arg) {
    u64 n = (u64)arg;
    int sum = 0;
    for (u64 i = 1; i <= n; i++) {
        sum += (int)i;
    }
    kthread_exit(sum);
}

/*
 * Full create -> run -> exit -> join lifecycle: the parent blocks in
 * thread_join() until the child terminates, then recovers its exit status.
 */
static test_result_t test_thread_join(void) {
    process_t* child = kthread_run(adder_child, (void*)10, WORKER_STACK_SIZE);
    TEST_ASSERT_NOT_NULL(child);

    int status = -1;
    TEST_ASSERT_EQ(thread_join(child, &status), 0);
    TEST_ASSERT_EQ(status, 55);   /* 1+2+...+10 */

    return TEST_PASS;
}

static volatile u64 sleep_bg_counter;
static volatile int sleep_bg_stop;

static void sleep_bg_worker(void* arg) {
    (void)arg;
    while (!sleep_bg_stop) {
        sleep_bg_counter++;
    }
}

/*
 * A blocking sleep must truly yield the CPU: while the main task sleeps for ~20
 * ticks, a background thread must keep running (proving the sleeper is off the
 * CPU, not busy-waiting), and the sleep must last about the requested duration.
 */
static test_result_t test_blocking_sleep(void) {
    sleep_bg_stop = 0;
    sleep_bg_counter = 0;

    process_t* bg = kthread_run(sleep_bg_worker, NULL, WORKER_STACK_SIZE);
    TEST_ASSERT_NOT_NULL(bg);

    u64 t0 = timer_get_ticks();
    sched_sleep(20);
    u64 dt = timer_get_ticks() - t0;
    u64 progressed = sleep_bg_counter;

    sleep_bg_stop = 1;
    TEST_ASSERT_EQ(thread_join(bg, NULL), 0);

    TEST_ASSERT(dt >= 18);          /* slept about the requested 20 ticks */
    TEST_ASSERT(dt <= 80);          /* and woke up in a timely manner */
    TEST_ASSERT_NE(progressed, 0);  /* the CPU ran the other thread meanwhile */

    return TEST_PASS;
}

/* Exits immediately; used to churn create/reap cycles. */
static void tiny_child(void* arg) {
    (void)arg;
    kthread_exit(0);
}

/*
 * Reaping must actually free a terminated task's stack: repeatedly creating and
 * joining threads must not monotonically drain physical memory (8 x 4 pages
 * would leak 32 pages without reaping).
 */
static test_result_t test_reaping_frees_memory(void) {
    size_t before = pmm_get_free_pages();

    for (int i = 0; i < 8; i++) {
        process_t* t = kthread_run(tiny_child, NULL, WORKER_STACK_SIZE);
        TEST_ASSERT_NOT_NULL(t);
        TEST_ASSERT_EQ(thread_join(t, NULL), 0);
    }

    size_t after = pmm_get_free_pages();
    TEST_ASSERT(after + 4 >= before);   /* no meaningful leak across 8 cycles */

    return TEST_PASS;
}

/* Register scheduler tests */
void register_scheduler_tests(void) {
    test_register("scheduler", "process_create", test_process_create);
    test_register("scheduler", "scheduler_stats", test_scheduler_stats);
    test_register("scheduler", "preemptive_concurrency", test_preemptive_concurrency);
    test_register("scheduler", "context_switch_integrity", test_context_switch_integrity);
    test_register("scheduler", "thread_join", test_thread_join);
    test_register("scheduler", "blocking_sleep", test_blocking_sleep);
    test_register("scheduler", "reaping_frees_memory", test_reaping_frees_memory);
}
