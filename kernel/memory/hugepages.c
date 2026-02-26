#include "hugepages.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_HUGEPAGES 1024

typedef struct hugepage {
    void* addr;
    size_t size;
    bool used;
} hugepage_t;

static hugepage_t hugepages[MAX_HUGEPAGES];
static u32 hugepage_count = 0;
static spinlock_t hugepage_lock = SPINLOCK_INIT;

void hugepages_init(void) {
    memset(hugepages, 0, sizeof(hugepages));
    DEBUG_INFO("Huge pages initialized");
}

void* hugepage_alloc(size_t size) {
    if (size != HUGEPAGE_2MB && size != HUGEPAGE_1GB) {
        return NULL;
    }
    
    spinlock_lock(&hugepage_lock);
    
    /* Find free huge page */
    for (u32 i = 0; i < hugepage_count; i++) {
        if (!hugepages[i].used && hugepages[i].size == size) {
            hugepages[i].used = true;
            spinlock_unlock(&hugepage_lock);
            return hugepages[i].addr;
        }
    }
    
    /* Allocate new huge page */
    if (hugepage_count < MAX_HUGEPAGES) {
        void* addr = vmm_alloc_pages((size + PAGE_SIZE - 1) / PAGE_SIZE);
        if (addr) {
            hugepages[hugepage_count].addr = addr;
            hugepages[hugepage_count].size = size;
            hugepages[hugepage_count].used = true;
            hugepage_count++;
            spinlock_unlock(&hugepage_lock);
            DEBUG_INFO("Huge page allocated: %u MB", (u32)(size / (1024 * 1024)));
            return addr;
        }
    }
    
    spinlock_unlock(&hugepage_lock);
    return NULL;
}

void hugepage_free(void* addr, size_t size) {
    if (!addr) return;
    
    spinlock_lock(&hugepage_lock);
    
    for (u32 i = 0; i < hugepage_count; i++) {
        if (hugepages[i].addr == addr && hugepages[i].size == size) {
            hugepages[i].used = false;
            spinlock_unlock(&hugepage_lock);
            return;
        }
    }
    
    spinlock_unlock(&hugepage_lock);
}

void hugepages_get_stats(u64* total, u64* free, u64* used) {
    if (!total || !free || !used) return;
    
    spinlock_lock(&hugepage_lock);
    
    *total = hugepage_count;
    *free = 0;
    *used = 0;
    
    for (u32 i = 0; i < hugepage_count; i++) {
        if (hugepages[i].used) {
            (*used)++;
        } else {
            (*free)++;
        }
    }
    
    spinlock_unlock(&hugepage_lock);
}
