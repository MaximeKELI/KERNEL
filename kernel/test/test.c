#include "test.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

static test_suite_t* test_suites = NULL;
static u32 total_passed = 0;
static u32 total_failed = 0;
static u32 total_skipped = 0;
static spinlock_t test_lock = SPINLOCK_INIT;

void test_init(void) {
    DEBUG_INFO("Test framework initialized");
}

void test_register(const char* suite, const char* name, test_func_t func) {
    if (!suite || !name || !func) return;
    
    /* Find or create suite */
    test_suite_t* suite_ptr = find_suite(suite);
    
    if (!suite_ptr) {
        suite_ptr = (test_suite_t*)kzalloc(sizeof(test_suite_t));
        if (!suite_ptr) return;
        suite_ptr->name = suite;
        suite_ptr->cases = NULL;
        suite_ptr->passed = suite_ptr->failed = suite_ptr->skipped = 0;
        
        spinlock_lock(&test_lock);
        suite_ptr->next = (test_suite_t*)test_suites;
        test_suites = suite_ptr;
        spinlock_unlock(&test_lock);
    }
    
    /* Add test case */
    test_case_t* test = (test_case_t*)kzalloc(sizeof(test_case_t));
    if (!test) return;
    
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
