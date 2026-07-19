#include "test.h"
#include "stdio.h"
#include "cache.h"
#include "epoll.h"
#include "vdso.h"

static test_result_t test_vdso_base(void) {
    return vdso_user_base() != 0 ? TEST_PASS : TEST_FAIL;
}

static test_result_t test_epoll_create(void) {
    epoll_init();
    int ep = epoll_create(8);
    return ep >= 0 ? TEST_PASS : TEST_FAIL;
}

static test_result_t test_page_cache_sync(void) {
    page_cache_t* p = page_cache_get(1, 0);
    if (!p) {
        return TEST_FAIL;
    }
    page_cache_mark_dirty(p);
    int r = page_cache_sync(p);
    page_cache_put(p);
    return r == 0 ? TEST_PASS : TEST_FAIL;
}

void register_phases_abcd_tests(void) {
    test_register("phases", "vdso", test_vdso_base);
    test_register("phases", "epoll", test_epoll_create);
    test_register("phases", "writeback", test_page_cache_sync);
}
