#include "test.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_TEST_SUITES 32
#define MAX_TESTS_PER_SUITE 64

static test_suite_t test_suites_array[MAX_TEST_SUITES];
static test_case_t test_cases_array[MAX_TEST_SUITES * MAX_TESTS_PER_SUITE];
static u32 next_suite = 0;
static u32 next_case = 0;
static u32 total_passed = 0;
static u32 total_failed = 0;
static u32 total_skipped = 0;
static spinlock_t test_lock = SPINLOCK_INIT;

void test_init(void) {
    DEBUG_INFO("Test framework initialized");
}

/* Helper to find suite */
static test_suite_t* find_suite(const char* name) {
    for (u32 i = 0; i < next_suite; i++) {
        if (strcmp(test_suites_array[i].name, name) == 0) {
            return &test_suites_array[i];
        }
    }
    return NULL;
}

void test_register(const char* suite, const char* name, test_func_t func) {
    if (!suite || !name || !func) return;
    
    /* Find or create suite */
    test_suite_t* suite_ptr = find_suite(suite);
    
    if (!suite_ptr) {
        if (next_suite >= MAX_TEST_SUITES) {
            DEBUG_ERROR("Too many test suites");
            return;
        }
        
        suite_ptr = &test_suites_array[next_suite++];
        suite_ptr->name = suite;
        suite_ptr->cases = NULL;
        suite_ptr->passed = suite_ptr->failed = suite_ptr->skipped = 0;
    }
    
    /* Add test case */
    if (next_case >= MAX_TEST_SUITES * MAX_TESTS_PER_SUITE) {
        DEBUG_ERROR("Too many test cases");
        return;
    }
    
    test_case_t* test = &test_cases_array[next_case++];
    test->name = name;
    test->suite = suite;
    test->func = func;
    
    spinlock_lock(&test_lock);
    test->next = suite_ptr->cases;
    suite_ptr->cases = test;
    spinlock_unlock(&test_lock);
}

int test_run_all(void) {
    printk("\n========================================\n");
    printk("  RUNNING ALL TESTS\n");
    printk("========================================\n\n");
    
    total_passed = total_failed = total_skipped = 0;
    
    for (u32 i = 0; i < next_suite; i++) {
        test_suite_t* suite = &test_suites_array[i];
        printk("Suite: %s\n", suite->name);
        printk("----------------------------------------\n");
        
        suite->passed = suite->failed = suite->skipped = 0;
        
        test_case_t* test = suite->cases;
        while (test) {
            printk("  [RUN] %s::%s ... ", test->suite, test->name);
            
            test_result_t result = test->func();
            
            switch (result) {
                case TEST_PASS:
                    printk("PASS\n");
                    suite->passed++;
                    total_passed++;
                    break;
                case TEST_FAIL:
                    printk("FAIL\n");
                    suite->failed++;
                    total_failed++;
                    break;
                case TEST_SKIP:
                    printk("SKIP\n");
                    suite->skipped++;
                    total_skipped++;
                    break;
            }
            
            test = test->next;
        }
        
        printk("\n  Results: %u passed, %u failed, %u skipped\n\n",
               suite->passed, suite->failed, suite->skipped);
    }
    
    printk("========================================\n");
    printk("  TEST SUMMARY\n");
    printk("========================================\n");
    printk("  Total: %u passed, %u failed, %u skipped\n",
           total_passed, total_failed, total_skipped);
    printk("========================================\n\n");
    
    return (total_failed == 0) ? 0 : -1;
}

int test_run_suite(const char* suite_name) {
    if (!suite_name) return -1;
    
    for (u32 i = 0; i < next_suite; i++) {
        test_suite_t* suite = &test_suites_array[i];
        if (strcmp(suite->name, suite_name) == 0) {
            printk("Running suite: %s\n", suite_name);
            /* Would run only this suite */
            return 0;
        }
    }
    
    return -1;
}
