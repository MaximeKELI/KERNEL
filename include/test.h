#ifndef TEST_H
#define TEST_H

#include "types.h"
#include "stdio.h"

/* Test result */
typedef enum {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP
} test_result_t;

/* Test function */
typedef test_result_t (*test_func_t)(void);

/* Test case */
typedef struct test_case {
    const char* name;
    const char* suite;
    test_func_t func;
    struct test_case* next;
} test_case_t;

/* Test suite */
typedef struct test_suite {
    const char* name;
    test_case_t* cases;
    u32 passed;
    u32 failed;
    u32 skipped;
} test_suite_t;

/* Initialize test framework */
void test_init(void);

/* Register test case */
void test_register(const char* suite, const char* name, test_func_t func);

/* Run all tests */
int test_run_all(void);

/* Run test suite */
int test_run_suite(const char* suite_name);

/* Assertions */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printk("[TEST FAIL] %s:%d: Assertion failed: %s\n", \
                   __FILE__, __LINE__, #condition); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printk("[TEST FAIL] %s:%d: Expected %lld, got %lld\n", \
                   __FILE__, __LINE__, (long long)(b), (long long)(a)); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            printk("[TEST FAIL] %s:%d: Values should not be equal\n", \
                   __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printk("[TEST FAIL] %s:%d: Pointer should be NULL\n", \
                   __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printk("[TEST FAIL] %s:%d: Pointer should not be NULL\n", \
                   __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printk("[TEST FAIL] %s:%d: Strings not equal: '%s' != '%s'\n", \
                   __FILE__, __LINE__, (a), (b)); \
            return TEST_FAIL; \
        } \
    } while(0)

#endif /* TEST_H */
