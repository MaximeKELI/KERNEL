#include "thp.h"
#include "memory.h"
#include "hugepages.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Transparent Huge Pages */
static bool thp_enabled = true;
static bool thp_defrag = true;
static spinlock_t thp_lock = SPINLOCK_INIT;
static u64 thp_allocations = 0;
static u64 thp_failures = 0;

void thp_init(void) {
    thp_enabled = true;
    thp_defrag = true;
    thp_allocations = 0;
    thp_failures = 0;
    printk("[THP] Transparent Huge Pages initialized\n");
}

void* thp_alloc(size_t size) {
    if (!thp_enabled || size < PAGE_SIZE_2MB) {
        return NULL;
    }
    
    /* Try to allocate huge page */
    size_t huge_pages = (size + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;
    void* addr = hugepage_alloc(huge_pages);
    
    spinlock_lock(&thp_lock);
    if (addr) {
        thp_allocations++;
    } else {
        thp_failures++;
        /* Fallback to regular pages */
        addr = vmm_alloc_pages((size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
    spinlock_unlock(&thp_lock);
    
    return addr;
}

void thp_free(void* addr, size_t size) {
    if (!addr || size == 0) {
        return;
    }
    
    /* Check if it's a huge page */
    size_t huge_pages = (size + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;
    
    if (hugepage_is_huge(addr)) {
        hugepage_free(addr, huge_pages);
    } else {
        vmm_free_pages(addr, (size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
}

void thp_enable(void) {
    spinlock_lock(&thp_lock);
    thp_enabled = true;
    spinlock_unlock(&thp_lock);
    printk("[THP] Enabled\n");
}

void thp_disable(void) {
    spinlock_lock(&thp_lock);
    thp_enabled = false;
    spinlock_unlock(&thp_lock);
    printk("[THP] Disabled\n");
}

bool thp_is_enabled(void) {
    spinlock_lock(&thp_lock);
    bool enabled = thp_enabled;
    spinlock_unlock(&thp_lock);
    return enabled;
}

void thp_set_defrag(bool enable) {
    spinlock_lock(&thp_lock);
    thp_defrag = enable;
    spinlock_unlock(&thp_lock);
}

bool thp_get_defrag(void) {
    spinlock_lock(&thp_lock);
    bool defrag = thp_defrag;
    spinlock_unlock(&thp_lock);
    return defrag;
}

void thp_get_stats(u64* allocations, u64* failures) {
    spinlock_lock(&thp_lock);
    if (allocations) *allocations = thp_allocations;
    if (failures) *failures = thp_failures;
    spinlock_unlock(&thp_lock);
}
