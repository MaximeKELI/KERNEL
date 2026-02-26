#include "test.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"

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

/* Register scheduler tests */
void register_scheduler_tests(void) {
    test_register("scheduler", "process_create", test_process_create);
    test_register("scheduler", "scheduler_stats", test_scheduler_stats);
}
