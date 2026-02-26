#include "memory_compaction.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "pmm.h"
#include "vmm.h"

static compact_stats_t compaction_stats = {0};
static spinlock_t compaction_lock = SPINLOCK_INIT;

static int compact_page_range(void* start, void* end, u32 mode) {
    u64 start_addr = (u64)start;
    u64 end_addr = (u64)end;
    u64 pages_migrated = 0;
    u64 pages_freed = 0;
    
    for (u64 addr = start_addr; addr < end_addr; addr += PAGE_SIZE) {
        if (!vmm_is_mapped((void*)addr)) {
            continue;
        }
        
        void* phys = vmm_get_phys((void*)addr);
        if (!phys) {
            continue;
        }
        
        /* Check if page is movable */
        /* Would check page flags here */
        
        /* Migrate page */
        void* new_phys = pmm_alloc(1);
        if (new_phys) {
            /* Copy page content */
            memcpy(new_phys, phys, PAGE_SIZE);
            
            /* Update mapping */
            vmm_unmap_page((void*)addr);
            vmm_map_page((void*)addr, new_phys, PAGE_PRESENT | PAGE_WRITABLE);
            
            /* Free old page */
            pmm_free(phys, 1);
            
            pages_migrated++;
            pages_freed++;
        }
        
        if (mode == COMPACT_MODE_LIGHT && pages_migrated > 100) {
            break; /* Limit migration in light mode */
        }
    }
    
    spinlock_lock(&compaction_lock);
    compaction_stats.pages_migrated += pages_migrated;
    compaction_stats.pages_freed += pages_freed;
    compaction_stats.compaction_events++;
    spinlock_unlock(&compaction_lock);
    
    return 0;
}

int compact_memory(u32 mode) {
    if (mode > COMPACT_MODE_AGGRESSIVE) {
        DEBUG_ERROR("Invalid compaction mode: %u", mode);
        return -1;
    }
    
    DEBUG_INFO("Starting memory compaction: mode=%u", mode);
    
    /* Compact kernel heap area */
    void* heap_start = (void*)0x200000;
    void* heap_end = (void*)0xC00000;
    compact_page_range(heap_start, heap_end, mode);
    
    DEBUG_INFO("Memory compaction completed");
    return 0;
}

void compact_get_stats(compact_stats_t* stats) {
    VALIDATE_PTR_VOID(stats);
    
    spinlock_lock(&compaction_lock);
    *stats = compaction_stats;
    spinlock_unlock(&compaction_lock);
}

void compact_on_pressure(void) {
    /* Trigger light compaction on memory pressure */
    extern size_t pmm_get_free_pages(void);
    extern size_t pmm_get_total_pages(void);
    
    size_t free = pmm_get_free_pages();
    size_t total = pmm_get_total_pages();
    
    if (total > 0 && (free * 100 / total) < 10) {
        DEBUG_INFO("Memory pressure detected, triggering compaction");
        compact_memory(COMPACT_MODE_LIGHT);
    }
}
