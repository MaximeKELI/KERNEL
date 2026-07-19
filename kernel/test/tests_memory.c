#include "test.h"
#include "memory.h"
#include "string.h"

/* Test PMM allocation */
static test_result_t test_pmm_alloc(void) {
    void* page1 = pmm_alloc(1);
    TEST_ASSERT_NOT_NULL(page1);
    
    void* page2 = pmm_alloc(1);
    TEST_ASSERT_NOT_NULL(page2);
    TEST_ASSERT_NE(page1, page2);
    
    pmm_free(page1, 1);
    pmm_free(page2, 1);
    
    return TEST_PASS;
}

/* Test heap allocation */
static test_result_t test_heap_alloc(void) {
    void* ptr1 = kmalloc(100);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    void* ptr2 = kmalloc(200);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NE(ptr1, ptr2);
    
    kfree(ptr1);
    kfree(ptr2);
    
    return TEST_PASS;
}

/* Test kzalloc */
static test_result_t test_kzalloc(void) {
    void* ptr = kzalloc(256);
    TEST_ASSERT_NOT_NULL(ptr);
    
    /* Check that memory is zeroed */
    u8* bytes = (u8*)ptr;
    for (u32 i = 0; i < 256; i++) {
        TEST_ASSERT_EQ(bytes[i], 0);
    }
    
    kfree(ptr);
    return TEST_PASS;
}

/* Test krealloc */
static test_result_t test_krealloc(void) {
    void* ptr = kmalloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    
    void* new_ptr = krealloc(ptr, 200);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    kfree(new_ptr);
    return TEST_PASS;
}

/* Test per-frame refcounting (COW safety): a shared frame is only freed once
 * the last sharer drops it, and the freed frame becomes reusable. */
static test_result_t test_pmm_refcount(void) {
    void* p = pmm_alloc(1);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQ(pmm_refcount(p), 1);          /* alloc starts at 1 */

    pmm_ref(p);
    TEST_ASSERT_EQ(pmm_refcount(p), 2);          /* second sharer */

    TEST_ASSERT_EQ(pmm_unref(p), 1);             /* one sharer left, not freed */
    TEST_ASSERT_EQ(pmm_refcount(p), 1);

    size_t free_before = pmm_get_free_pages();
    TEST_ASSERT_EQ(pmm_unref(p), 0);             /* last sharer -> freed */
    TEST_ASSERT_EQ(pmm_refcount(p), 0);
    TEST_ASSERT(pmm_get_free_pages() > free_before);

    /* The freed frame must be reusable. */
    void* q = pmm_alloc(1);
    TEST_ASSERT_NOT_NULL(q);
    TEST_ASSERT_EQ(pmm_refcount(q), 1);
    pmm_free(q, 1);
    return TEST_PASS;
}

/* Register memory tests */
void register_memory_tests(void) {
    test_register("memory", "pmm_alloc", test_pmm_alloc);
    test_register("memory", "pmm_refcount", test_pmm_refcount);
    test_register("memory", "heap_alloc", test_heap_alloc);
    test_register("memory", "kzalloc", test_kzalloc);
    test_register("memory", "krealloc", test_krealloc);
}
