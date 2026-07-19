#include "test.h"
#include "memory.h"
#include "mm.h"
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

/*
 * A high, canonical user VA that lands in a PML4 slot the kernel never uses
 * (0x100000000000 >> 39 == 32), so mapping it in a test address space never
 * disturbs the shared identity/kernel subtrees.
 */
#define TEST_USER_VA ((void*)0x100000000000ULL)

static inline u64 irq_save_off(void) {
    u64 fl;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(fl) :: "memory");
    return fl;
}
static inline void irq_restore(u64 fl) {
    if (fl & (1ULL << 9)) {
        __asm__ volatile("sti" ::: "memory");
    }
}

/* Test address-space isolation: two spaces, same VA, different physical frames. */
static test_result_t test_vmm_isolation(void) {
    u64 fl = irq_save_off();
    u64 save = vmm_get_cr3();
    u64 as1 = vmm_create_user_space();
    u64 as2 = vmm_create_user_space();
    void* p1 = pmm_alloc(1);
    void* p2 = pmm_alloc(1);
    u64 v1 = 0, v2 = 0;
    int ok = (as1 && as2 && p1 && p2 && p1 != p2);

    if (ok) {
        vmm_switch_mm(as1);
        vmm_map_page(TEST_USER_VA, p1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        *(volatile u64*)TEST_USER_VA = 0xAAAA1111ULL;

        vmm_switch_mm(as2);
        vmm_map_page(TEST_USER_VA, p2, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        *(volatile u64*)TEST_USER_VA = 0xBBBB2222ULL;

        vmm_switch_mm(as1);
        v1 = *(volatile u64*)TEST_USER_VA;
        vmm_switch_mm(as2);
        v2 = *(volatile u64*)TEST_USER_VA;
    }

    vmm_switch_mm(save);
    if (as1) vmm_destroy_user_space(as1);
    if (as2) vmm_destroy_user_space(as2);
    irq_restore(fl);

    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(v1, 0xAAAA1111ULL);   /* as1 kept its own value */
    TEST_ASSERT_EQ(v2, 0xBBBB2222ULL);   /* as2 unaffected by as1's write */
    return TEST_PASS;
}

/* Test copy-on-write fork: shared read-only frame, private copy on write,
 * refcount transitions 1 -> 2 -> 1, parent invisible to child and vice versa. */
static test_result_t test_vmm_cow_fork(void) {
    u64 fl = irq_save_off();
    /* Force CR0.WP so a ring-0 write to a read-only page faults (as ring-3 would),
     * exercising the real page-fault -> COW path. */
    u64 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0 | (1ULL << 16)) : "memory");

    u64 save = vmm_get_cr3();
    u64 parent = vmm_create_user_space();
    void* pp = pmm_alloc(1);
    unsigned rc1 = 0, rc2 = 0, rc3 = 0;
    u64 child_pre = 0, child_val = 0, parent_val = 0;
    u64 child = 0;
    int ok = (parent && pp);

    if (ok) {
        vmm_switch_mm(parent);
        vmm_map_page(TEST_USER_VA, pp, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        *(volatile u64*)TEST_USER_VA = 0x1234ULL;
        rc1 = pmm_refcount(pp);              /* 1: only the parent */

        child = vmm_fork_from(parent);
        ok = (child != 0);
        rc2 = pmm_refcount(pp);              /* 2: shared COW with the child */

        if (ok) {
            vmm_switch_mm(child);
            child_pre = *(volatile u64*)TEST_USER_VA;   /* reads 0x1234 read-only */
            *(volatile u64*)TEST_USER_VA = 0x5678ULL;   /* write -> COW fault */
            child_val = *(volatile u64*)TEST_USER_VA;
            rc3 = pmm_refcount(pp);          /* back to 1: child got its own copy */

            vmm_switch_mm(parent);
            parent_val = *(volatile u64*)TEST_USER_VA;  /* still 0x1234 */
        }
    }

    vmm_switch_mm(save);
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");   /* restore WP */
    if (child) vmm_destroy_user_space(child);
    if (parent) vmm_destroy_user_space(parent);
    irq_restore(fl);

    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(rc1, 1u);
    TEST_ASSERT_EQ(rc2, 2u);
    TEST_ASSERT_EQ(child_pre, 0x1234ULL);
    TEST_ASSERT_EQ(child_val, 0x5678ULL);
    TEST_ASSERT_EQ(rc3, 1u);
    TEST_ASSERT_EQ(parent_val, 0x1234ULL);   /* parent write-isolated from child */
    return TEST_PASS;
}

/* Regression: the heap must live entirely above the kernel image, never
 * overlapping kernel BSS (TSS ring-0 stack / descriptor tables). A large
 * allocation must stay within the reported heap range. */
static test_result_t test_heap_above_kernel(void) {
    extern u8 _kernel_end[];
    u64 start = 0, end = 0;
    heap_get_range(&start, &end);
    TEST_ASSERT(start >= (u64)(uintptr_t)_kernel_end);
    TEST_ASSERT(end > start);

    void* p = kmalloc(256 * 1024);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT((u64)p >= start && (u64)p < end);
    kfree(p);
    return TEST_PASS;
}

/* Test VMA bookkeeping: insert, find, and munmap-style split of the middle. */
static test_result_t test_mm_vma_ops(void) {
    mm_struct_t* mm = mm_create();
    TEST_ASSERT_NOT_NULL(mm);

    mm_insert_vma(mm, 0x10000, 0x20000, VM_READ | VM_WRITE);
    TEST_ASSERT_NOT_NULL(mm_find_vma(mm, 0x10000));
    TEST_ASSERT_NOT_NULL(mm_find_vma(mm, 0x1F000));
    TEST_ASSERT(mm_find_vma(mm, 0x20000) == NULL);   /* end is exclusive */

    /* Punch a hole in the middle -> split into two VMAs. */
    mm_remove_range(mm, 0x14000, 0x16000);
    TEST_ASSERT_NOT_NULL(mm_find_vma(mm, 0x13000));
    TEST_ASSERT(mm_find_vma(mm, 0x15000) == NULL);   /* hole */
    TEST_ASSERT_NOT_NULL(mm_find_vma(mm, 0x17000));

    mm_destroy(mm);
    return TEST_PASS;
}

/* Test brk() growth/shrink bookkeeping and range enforcement. */
static test_result_t test_mm_brk(void) {
    mm_struct_t* mm = mm_create();
    TEST_ASSERT_NOT_NULL(mm);

    TEST_ASSERT_EQ(mm_brk(mm, 0), USER_BRK_BASE);              /* query */
    TEST_ASSERT_EQ(mm_brk(mm, USER_BRK_BASE + 0x4000), USER_BRK_BASE + 0x4000);
    vma_t* heap = mm_find_vma(mm, USER_BRK_BASE);
    TEST_ASSERT_NOT_NULL(heap);
    TEST_ASSERT(heap->end >= USER_BRK_BASE + 0x4000);

    TEST_ASSERT_EQ(mm_brk(mm, USER_BRK_BASE + 0x1000), USER_BRK_BASE + 0x1000); /* shrink */
    TEST_ASSERT_EQ(mm_brk(mm, USER_BRK_MAX + 0x1000), USER_BRK_BASE + 0x1000);  /* refuse */

    mm_destroy(mm);
    return TEST_PASS;
}

/* Test mprotect() splitting a VMA and flipping write permission. */
static test_result_t test_mm_mprotect(void) {
    mm_struct_t* mm = mm_create();
    TEST_ASSERT_NOT_NULL(mm);

    mm_insert_vma(mm, 0x30000, 0x33000, VM_READ | VM_WRITE);
    TEST_ASSERT_EQ(mm_protect_range(mm, 0x30000, 0x31000, VM_READ), 0);

    vma_t* ro = mm_find_vma(mm, 0x30000);
    vma_t* rw = mm_find_vma(mm, 0x31000);
    TEST_ASSERT_NOT_NULL(ro);
    TEST_ASSERT_NOT_NULL(rw);
    TEST_ASSERT((ro->vm_flags & VM_WRITE) == 0);   /* first page now read-only */
    TEST_ASSERT((rw->vm_flags & VM_WRITE) != 0);   /* rest still writable */

    mm_destroy(mm);
    return TEST_PASS;
}

/* Test the fault dispatcher: anonymous demand-zero, out-of-VMA rejection, and
 * stack growth. Runs in a private address space at a high, unshared VA. */
static test_result_t test_mm_demand_and_stack(void) {
    const u64 VA = 0x100000000000ULL;         /* anon region */
    const u64 SBASE = 0x100000200000ULL;      /* stack VMA base */
    u64 fl = irq_save_off();
    u64 save = vmm_get_cr3();
    u64 as = vmm_create_user_space();
    mm_struct_t* mm = mm_create();
    int ok = (as && mm);
    u64 v0 = 1, v1 = 1;
    int oob = 0, grew = 0, mapped_below = 0;
    u64 grown_start = 0;

    if (ok) {
        vmm_switch_mm(as);

        /* Anonymous demand-zero over two pages. */
        mm_insert_vma(mm, VA, VA + 0x2000, VM_READ | VM_WRITE | VM_ANON);
        ok = (mm_fault(mm, VA, 0x2) == 0);        /* write fault, not present */
        if (ok) {
            v0 = *(volatile u64*)VA;              /* must read as zero */
            *(volatile u64*)VA = 0xC0FFEE;
            v1 = *(volatile u64*)VA;
        }
        /* A fault with no backing VMA must be rejected (no crash: direct call). */
        oob = (mm_fault(mm, VA + 0x800000, 0x2) == -1);

        /* Stack growth: fault one page below a GROWSDOWN VMA. */
        mm_insert_vma(mm, SBASE, SBASE + 0x2000,
                      VM_READ | VM_WRITE | VM_ANON | VM_GROWSDOWN);
        grew = (mm_fault(mm, SBASE - 0x1000, 0x2) == 0);
        vma_t* s = mm_find_vma(mm, SBASE - 0x1000);
        grown_start = s ? s->start : 0;
        mapped_below = vmm_is_mapped((void*)(SBASE - 0x1000));
    }

    vmm_switch_mm(save);
    if (as) vmm_destroy_user_space(as);
    if (mm) mm_destroy(mm);
    irq_restore(fl);

    TEST_ASSERT(ok);
    TEST_ASSERT_EQ(v0, 0ULL);              /* demand-zero */
    TEST_ASSERT_EQ(v1, 0xC0FFEEULL);       /* writable */
    TEST_ASSERT(oob);                      /* out-of-VMA rejected */
    TEST_ASSERT(grew);
    TEST_ASSERT_EQ(grown_start, SBASE - 0x1000);
    TEST_ASSERT(mapped_below);
    return TEST_PASS;
}

/* Register memory tests */
void register_memory_tests(void) {
    test_register("memory", "pmm_alloc", test_pmm_alloc);
    test_register("memory", "pmm_refcount", test_pmm_refcount);
    test_register("memory", "heap_above_kernel", test_heap_above_kernel);
    test_register("memory", "vmm_isolation", test_vmm_isolation);
    test_register("memory", "vmm_cow_fork", test_vmm_cow_fork);
    test_register("memory", "mm_vma_ops", test_mm_vma_ops);
    test_register("memory", "mm_brk", test_mm_brk);
    test_register("memory", "mm_mprotect", test_mm_mprotect);
    test_register("memory", "mm_demand_and_stack", test_mm_demand_and_stack);
    test_register("memory", "heap_alloc", test_heap_alloc);
    test_register("memory", "kzalloc", test_kzalloc);
    test_register("memory", "krealloc", test_krealloc);
}
