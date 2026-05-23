#include "test.h"
#include "stdio.h"
#include "cache.h"
#include "epoll.h"
#include "vdso.h"

static int test_vdso_base(void) {
    return vdso_user_base() != 0 ? 0 : -1;
}

static int test_epoll_create(void) {
    epoll_init();
    int ep = epoll_create(8);
    return ep >= 0 ? 0 : -1;
}

static int test_page_cache_sync(void) {
    page_cache_t* p = page_cache_get(1, 0);
    if (!p) {
        return -1;
    }
    page_cache_mark_dirty(p);
    int r = page_cache_sync(p);
    page_cache_put(p);
    return r == 0 ? 0 : -1;
}

void register_phases_abcd_tests(void) {
    test_register("phases", "vdso", test_vdso_base);
    test_register("phases", "epoll", test_epoll_create);
    test_register("phases", "writeback", test_page_cache_sync);
}
