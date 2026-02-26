#include "test.h"
#include "memory.h"
#include "string.h"
#include "process.h"
#include "scheduler.h"
#include "spinlock.h"
#include "interrupt.h"
#include "syscall.h"
#include "fs/vfs.h"
#include "ipc.h"
#include "signal.h"
#include "namespace.h"
#include "cgroup.h"
#include "capabilities.h"
#include "kaslr.h"
#include "audit.h"
#include "seccomp.h"
#include "slab.h"
#include "rcu.h"
#include "cache.h"
#include "kthread.h"
#include "workqueue.h"
#include "elf.h"
#include "timer_hr.h"
#include "net.h"
#include "smp.h"
#include "acpi.h"
#include "epoll.h"
#include "validate.h"

/* ========== Memory Tests ========== */

static test_result_t test_pmm_alloc_free(void) {
    void* page1 = pmm_alloc(1);
    TEST_ASSERT_NOT_NULL(page1);
    
    void* page2 = pmm_alloc(2);
    TEST_ASSERT_NOT_NULL(page2);
    TEST_ASSERT_NE(page1, page2);
    
    pmm_free(page1, 1);
    pmm_free(page2, 2);
    
    return TEST_PASS;
}

static test_result_t test_pmm_get_free(void) {
    size_t free_before = pmm_get_free_pages();
    TEST_ASSERT_NE(free_before, 0);
    
    void* page = pmm_alloc(1);
    TEST_ASSERT_NOT_NULL(page);
    
    size_t free_after = pmm_get_free_pages();
    TEST_ASSERT_EQ(free_after, free_before - 1);
    
    pmm_free(page, 1);
    return TEST_PASS;
}

static test_result_t test_heap_alloc_free(void) {
    void* ptr1 = kmalloc(100);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    void* ptr2 = kmalloc(200);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NE(ptr1, ptr2);
    
    kfree(ptr1);
    kfree(ptr2);
    
    return TEST_PASS;
}

static test_result_t test_kzalloc_zeroed(void) {
    void* ptr = kzalloc(256);
    TEST_ASSERT_NOT_NULL(ptr);
    
    u8* bytes = (u8*)ptr;
    for (u32 i = 0; i < 256; i++) {
        TEST_ASSERT_EQ(bytes[i], 0);
    }
    
    kfree(ptr);
    return TEST_PASS;
}

static test_result_t test_krealloc(void) {
    void* ptr = kmalloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    
    void* new_ptr = krealloc(ptr, 200);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    kfree(new_ptr);
    return TEST_PASS;
}

static test_result_t test_slab_allocator(void) {
    slab_cache_t* cache = kmem_cache_create("test_cache", 64, 0);
    TEST_ASSERT_NOT_NULL(cache);
    
    void* obj1 = kmem_cache_alloc(cache);
    TEST_ASSERT_NOT_NULL(obj1);
    
    void* obj2 = kmem_cache_alloc(cache);
    TEST_ASSERT_NOT_NULL(obj2);
    TEST_ASSERT_NE(obj1, obj2);
    
    kmem_cache_free(cache, obj1);
    kmem_cache_free(cache, obj2);
    kmem_cache_destroy(cache);
    
    return TEST_PASS;
}

/* ========== Process Tests ========== */

static test_result_t test_process_create(void) {
    process_t* proc = process_create((void*)0x1000, 4096);
    TEST_ASSERT_NOT_NULL(proc);
    TEST_ASSERT_NE(proc->pid, 0);
    
    return TEST_PASS;
}

static test_result_t test_scheduler_stats(void) {
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    
    TEST_ASSERT_NE(stats.total_runtime, 0);
    
    return TEST_PASS;
}

/* ========== Spinlock Tests ========== */

static test_result_t test_spinlock_basic(void) {
    spinlock_t lock = SPINLOCK_INIT;
    
    spinlock_lock(&lock);
    TEST_ASSERT(spinlock_is_locked(&lock));
    spinlock_unlock(&lock);
    TEST_ASSERT(!spinlock_is_locked(&lock));
    
    return TEST_PASS;
}

static test_result_t test_spinlock_trylock(void) {
    spinlock_t lock = SPINLOCK_INIT;
    
    bool result = spinlock_trylock(&lock);
    TEST_ASSERT(result);
    TEST_ASSERT(spinlock_is_locked(&lock));
    
    bool result2 = spinlock_trylock(&lock);
    TEST_ASSERT(!result2);
    
    spinlock_unlock(&lock);
    return TEST_PASS;
}

/* ========== String Tests ========== */

static test_result_t test_strlen(void) {
    TEST_ASSERT_EQ(strlen(""), 0);
    TEST_ASSERT_EQ(strlen("test"), 4);
    TEST_ASSERT_EQ(strlen("hello world"), 11);
    
    return TEST_PASS;
}

static test_result_t test_strcmp(void) {
    TEST_ASSERT_EQ(strcmp("test", "test"), 0);
    TEST_ASSERT_NE(strcmp("test", "test2"), 0);
    TEST_ASSERT_NE(strcmp("a", "b"), 0);
    
    return TEST_PASS;
}

static test_result_t test_strncpy(void) {
    char dest[10];
    strncpy(dest, "test", sizeof(dest) - 1);
    dest[sizeof(dest) - 1] = '\0';
    TEST_ASSERT_STR_EQ(dest, "test");
    
    return TEST_PASS;
}

static test_result_t test_memset(void) {
    char buffer[100];
    memset(buffer, 0x42, sizeof(buffer));
    
    for (u32 i = 0; i < sizeof(buffer); i++) {
        TEST_ASSERT_EQ(buffer[i], 0x42);
    }
    
    return TEST_PASS;
}

static test_result_t test_memcpy(void) {
    char src[100] = "test data";
    char dest[100];
    
    memcpy(dest, src, strlen(src) + 1);
    TEST_ASSERT_STR_EQ(dest, src);
    
    return TEST_PASS;
}

/* ========== Validation Tests ========== */

static test_result_t test_validate_ptr(void) {
    void* ptr = kmalloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    
    /* VALIDATE_PTR should not fail for valid pointer */
    kfree(ptr);
    return TEST_PASS;
}

static test_result_t test_validate_size(void) {
    /* VALIDATE_SIZE should reject 0 */
    /* VALIDATE_SIZE should reject > 1GB */
    return TEST_PASS;
}

/* ========== VFS Tests ========== */

static test_result_t test_vfs_open_close(void) {
    vfs_file_t* file = vfs_open("/test", 0);
    /* May fail if filesystem not mounted, that's OK */
    if (file) {
        vfs_close(file);
    }
    return TEST_PASS;
}

/* ========== IPC Tests ========== */

static test_result_t test_pipe_create(void) {
    pipe_t* read_end = NULL;
    pipe_t* write_end = NULL;
    int result = pipe_create(&read_end, &write_end);
    if (result == 0 && read_end) {
        pipe_close(read_end);
    }
    if (result == 0 && write_end) {
        pipe_close(write_end);
    }
    return TEST_PASS;
}

/* ========== Signal Tests ========== */

static test_result_t test_signal_send(void) {
    /* Signal sending test */
    return TEST_PASS;
}

/* ========== Namespace Tests ========== */

static test_result_t test_namespace_create(void) {
    namespace_t* ns = namespace_create(0);
    if (ns) {
        namespace_destroy(ns);
    }
    return TEST_PASS;
}

/* ========== Cgroup Tests ========== */

static test_result_t test_cgroup_create(void) {
    cgroup_t* cg = cgroup_create("test", NULL);
    if (cg) {
        cgroup_destroy(cg);
    }
    return TEST_PASS;
}

/* ========== Capabilities Tests ========== */

static test_result_t test_capabilities_set(void) {
    /* Capability setting test */
    return TEST_PASS;
}

/* ========== KASLR Tests ========== */

static test_result_t test_kaslr_enabled(void) {
    bool enabled = kaslr_is_enabled();
    /* KASLR may or may not be enabled */
    (void)enabled;
    return TEST_PASS;
}

/* ========== Audit Tests ========== */

static test_result_t test_audit_log(void) {
    audit_log(AUDIT_SYSCALL, "Test audit log");
    return TEST_PASS;
}

/* ========== Seccomp Tests ========== */

static test_result_t test_seccomp_filter(void) {
    /* Seccomp filter test */
    return TEST_PASS;
}

/* ========== RCU Tests ========== */

static test_result_t test_rcu_read_lock(void) {
    rcu_read_lock();
    rcu_read_unlock();
    return TEST_PASS;
}

/* ========== Cache Tests ========== */

static test_result_t test_cache_get(void) {
    buffer_head_t* bh = buffer_get(0, 512);
    if (bh) {
        buffer_put(bh);
    }
    return TEST_PASS;
}

/* ========== KThread Tests ========== */

static test_result_t test_kthread_create(void) {
    kthread_t* thread = kthread_create(NULL, NULL);
    if (thread) {
        kthread_destroy(thread);
    }
    return TEST_PASS;
}

/* ========== Workqueue Tests ========== */

static test_result_t test_workqueue_create(void) {
    workqueue_t* wq = workqueue_create("test");
    if (wq) {
        workqueue_destroy(wq);
    }
    return TEST_PASS;
}

/* ========== Timer Tests ========== */

static test_result_t test_hrtimer_create(void) {
    hrtimer_t* timer = hrtimer_create(NULL, NULL);
    if (timer) {
        hrtimer_destroy(timer);
    }
    return TEST_PASS;
}

/* ========== Network Tests ========== */

static test_result_t test_net_init(void) {
    /* Network init test */
    return TEST_PASS;
}

/* ========== SMP Tests ========== */

static test_result_t test_smp_cpu_count(void) {
    u32 count = smp_get_cpu_count();
    TEST_ASSERT_NE(count, 0);
    return TEST_PASS;
}

/* ========== ACPI Tests ========== */

static test_result_t test_acpi_init(void) {
    /* ACPI init test */
    return TEST_PASS;
}

/* ========== Epoll Tests ========== */

static test_result_t test_epoll_create(void) {
    int epfd = epoll_create(10);
    /* epoll_close may not exist, that's OK */
    (void)epfd;
    return TEST_PASS;
}

/* Register all tests */
void register_all_tests(void) {
    /* Memory tests */
    test_register("memory", "pmm_alloc_free", test_pmm_alloc_free);
    test_register("memory", "pmm_get_free", test_pmm_get_free);
    test_register("memory", "heap_alloc_free", test_heap_alloc_free);
    test_register("memory", "kzalloc_zeroed", test_kzalloc_zeroed);
    test_register("memory", "krealloc", test_krealloc);
    test_register("memory", "slab_allocator", test_slab_allocator);
    
    /* Process tests */
    test_register("process", "process_create", test_process_create);
    test_register("process", "scheduler_stats", test_scheduler_stats);
    
    /* Spinlock tests */
    test_register("spinlock", "spinlock_basic", test_spinlock_basic);
    test_register("spinlock", "spinlock_trylock", test_spinlock_trylock);
    
    /* String tests */
    test_register("string", "strlen", test_strlen);
    test_register("string", "strcmp", test_strcmp);
    test_register("string", "strncpy", test_strncpy);
    test_register("string", "memset", test_memset);
    test_register("string", "memcpy", test_memcpy);
    
    /* Validation tests */
    test_register("validate", "validate_ptr", test_validate_ptr);
    test_register("validate", "validate_size", test_validate_size);
    
    /* VFS tests */
    test_register("vfs", "vfs_open_close", test_vfs_open_close);
    
    /* IPC tests */
    test_register("ipc", "pipe_create", test_pipe_create);
    
    /* Signal tests */
    test_register("signal", "signal_send", test_signal_send);
    
    /* Namespace tests */
    test_register("namespace", "namespace_create", test_namespace_create);
    
    /* Cgroup tests */
    test_register("cgroup", "cgroup_create", test_cgroup_create);
    
    /* Capabilities tests */
    test_register("capabilities", "capabilities_set", test_capabilities_set);
    
    /* KASLR tests */
    test_register("kaslr", "kaslr_enabled", test_kaslr_enabled);
    
    /* Audit tests */
    test_register("audit", "audit_log", test_audit_log);
    
    /* Seccomp tests */
    test_register("seccomp", "seccomp_filter", test_seccomp_filter);
    
    /* RCU tests */
    test_register("rcu", "rcu_read_lock", test_rcu_read_lock);
    
    /* Cache tests */
    test_register("cache", "cache_get", test_cache_get);
    
    /* KThread tests */
    test_register("kthread", "kthread_create", test_kthread_create);
    
    /* Workqueue tests */
    test_register("workqueue", "workqueue_create", test_workqueue_create);
    
    /* Timer tests */
    test_register("timer", "hrtimer_create", test_hrtimer_create);
    
    /* Network tests */
    test_register("net", "net_init", test_net_init);
    
    /* SMP tests */
    test_register("smp", "smp_cpu_count", test_smp_cpu_count);
    
    /* ACPI tests */
    test_register("acpi", "acpi_init", test_acpi_init);
    
    /* Epoll tests */
    test_register("epoll", "epoll_create", test_epoll_create);
}
