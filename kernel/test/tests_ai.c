#include "test.h"
#include "ai_manager.h"
#include "ai_monitor.h"
#include "ai_optimizer.h"
#include "ai_policy.h"
#include "ai_predict.h"
#include "ai_log.h"
#include "memory.h"
#include "process.h"
#include "scheduler.h"
#include "stdio.h"

/* Test AI initialization */
static test_result_t test_ai_init(void) {
    /* Initialize AI subsystem */
    ai_init();
    
    /* Check that it's initialized */
    extern bool ai_initialized;
    TEST_ASSERT_TRUE(ai_initialized);
    
    return TEST_PASS;
}

/* Test AI metrics structure */
static test_result_t test_ai_metrics_structure(void) {
    ai_metrics_t metrics = {0};
    
    /* Initialize metrics */
    metrics.cpu_usage = 50;
    metrics.memory_usage = 60;
    metrics.process_count = 5;
    metrics.context_switches = 100;
    metrics.interrupt_rate = 200;
    
    /* Verify values */
    TEST_ASSERT_EQ(metrics.cpu_usage, 50);
    TEST_ASSERT_EQ(metrics.memory_usage, 60);
    TEST_ASSERT_EQ(metrics.process_count, 5);
    TEST_ASSERT_EQ(metrics.context_switches, 100);
    TEST_ASSERT_EQ(metrics.interrupt_rate, 200);
    
    return TEST_PASS;
}

/* Test AI monitor initialization */
static test_result_t test_ai_monitor_init(void) {
    ai_monitor_init();
    
    /* Get metrics - should be zero initially */
    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    
    /* Verify initial state */
    TEST_ASSERT_EQ(metrics.cpu_usage, 0);
    TEST_ASSERT_EQ(metrics.memory_usage, 0);
    TEST_ASSERT_EQ(metrics.process_count, 0);
    
    return TEST_PASS;
}

/* Test AI monitor update */
static test_result_t test_ai_monitor_update(void) {
    /* Initialize monitor */
    ai_monitor_init();
    
    /* Update metrics */
    ai_update_metrics();
    
    /* Get metrics */
    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    
    /* Verify metrics are updated (at least process_count should be valid) */
    TEST_ASSERT_TRUE(metrics.process_count >= 0);
    TEST_ASSERT_TRUE(metrics.memory_usage >= 0);
    TEST_ASSERT_TRUE(metrics.memory_usage <= 100);
    TEST_ASSERT_TRUE(metrics.cpu_usage >= 0);
    TEST_ASSERT_TRUE(metrics.cpu_usage <= 100);
    
    return TEST_PASS;
}

/* Test AI optimizer initialization */
static test_result_t test_ai_optimizer_init(void) {
    ai_optimizer_init();
    
    /* Optimizer should be initialized */
    /* We can't directly check internal state, but we can verify it doesn't crash */
    ai_optimize_scheduler();
    ai_optimize_memory();
    ai_detect_anomalies();
    
    return TEST_PASS;
}

/* Test AI get metrics */
static test_result_t test_ai_get_metrics(void) {
    /* Initialize AI */
    ai_init();
    
    /* Get metrics */
    ai_metrics_t metrics;
    ai_get_metrics(&metrics);
    
    /* Verify metrics structure is valid */
    TEST_ASSERT_TRUE(metrics.cpu_usage >= 0);
    TEST_ASSERT_TRUE(metrics.cpu_usage <= 100);
    TEST_ASSERT_TRUE(metrics.memory_usage >= 0);
    TEST_ASSERT_TRUE(metrics.memory_usage <= 100);
    TEST_ASSERT_TRUE(metrics.process_count >= 0);
    
    return TEST_PASS;
}

/* Test AI tick */
static test_result_t test_ai_tick(void) {
    /* Initialize AI */
    ai_init();
    
    /* Call tick - should not crash */
    ai_tick();
    
    /* Verify metrics are updated */
    ai_metrics_t metrics;
    ai_get_metrics(&metrics);
    
    /* Metrics should be valid */
    TEST_ASSERT_TRUE(metrics.process_count >= 0);
    
    return TEST_PASS;
}

/* Test AI with null pointer */
static test_result_t test_ai_get_metrics_null(void) {
    /* Should handle null pointer gracefully */
    ai_get_metrics(NULL);
    
    /* Should not crash */
    return TEST_PASS;
}

static test_result_t test_ai_policy_modes(void) {
    ai_policy_init();
    ai_policy_set_mode(AI_POLICY_LATENCY);
    TEST_ASSERT_EQ(ai_policy_get_mode(), AI_POLICY_LATENCY);
    ai_policy_set_mode(AI_POLICY_THROUGHPUT);
    TEST_ASSERT_EQ(ai_policy_get_mode(), AI_POLICY_THROUGHPUT);
    return TEST_PASS;
}

static test_result_t test_ai_predict_ema(void) {
    ai_predict_init();
    ai_metrics_t m = { .cpu_usage = 40, .memory_usage = 50 };
    ai_predict_feed(&m);
    m.cpu_usage = 60;
    ai_predict_feed(&m);
    u64 pred = ai_predict_cpu();
    TEST_ASSERT_TRUE(pred >= 40);
    return TEST_PASS;
}

static test_result_t test_ai_log_ring(void) {
    ai_log_init();
    ai_log_record(AI_ACT_PRIO_BOOST, 1, 5);
    TEST_ASSERT_TRUE(ai_log_count() >= 1);
    ai_log_clear();
    TEST_ASSERT_EQ(ai_log_count(), 0);
    return TEST_PASS;
}

/* Register AI tests */
void register_ai_tests(void) {
    test_register("ai", "ai_init", test_ai_init);
    test_register("ai", "ai_metrics_structure", test_ai_metrics_structure);
    test_register("ai", "ai_monitor_init", test_ai_monitor_init);
    test_register("ai", "ai_monitor_update", test_ai_monitor_update);
    test_register("ai", "ai_optimizer_init", test_ai_optimizer_init);
    test_register("ai", "ai_get_metrics", test_ai_get_metrics);
    test_register("ai", "ai_tick", test_ai_tick);
    test_register("ai", "ai_get_metrics_null", test_ai_get_metrics_null);
    test_register("ai", "ai_policy_modes", test_ai_policy_modes);
    test_register("ai", "ai_predict_ema", test_ai_predict_ema);
    test_register("ai", "ai_log_ring", test_ai_log_ring);
}
