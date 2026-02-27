#include "memory_defrag.h"
#include "memory.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

static bool defrag_enabled = false;
static spinlock_t defrag_lock = SPINLOCK_INIT;
static u64 defrag_operations = 0;
static u64 defrag_pages_moved = 0;

void memory_defrag_init(void) {
    defrag_enabled = true;
    defrag_operations = 0;
    defrag_pages_moved = 0;
    printk("[Memory Defrag] Initialized\n");
}

u64 memory_defrag_region(void* start, size_t size) {
    if (!defrag_enabled || !start || size == 0) {
        return 0;
    }
    
    spinlock_lock(&defrag_lock);
    
    /* Simplified defragmentation: move pages to contiguous locations */
    u64 pages_moved = 0;
    u64 page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Find contiguous free space */
    void* new_location = pmm_alloc(page_count);
    if (new_location) {
        /* Move pages (simplified - would need proper page copying) */
        pages_moved = page_count;
        defrag_pages_moved += pages_moved;
        defrag_operations++;
        
        /* Free old location */
        pmm_free(start, page_count);
    }
    
    spinlock_unlock(&defrag_lock);
    
    return pages_moved;
}

u64 memory_defrag_system(void) {
    if (!defrag_enabled) {
        return 0;
    }
    
    /* System-wide defragmentation */
    u64 total_moved = 0;
    
    /* TODO: Iterate through memory regions and defragment */
    /* This is a placeholder - real implementation would scan memory */
    
    return total_moved;
}

void memory_defrag_enable(void) {
    spinlock_lock(&defrag_lock);
    defrag_enabled = true;
    spinlock_unlock(&defrag_lock);
}

void memory_defrag_disable(void) {
    spinlock_lock(&defrag_lock);
    defrag_enabled = false;
    spinlock_unlock(&defrag_lock);
}

bool memory_defrag_is_enabled(void) {
    spinlock_lock(&defrag_lock);
    bool enabled = defrag_enabled;
    spinlock_unlock(&defrag_lock);
    return enabled;
}

void memory_defrag_get_stats(u64* operations, u64* pages_moved) {
    spinlock_lock(&defrag_lock);
    if (operations) *operations = defrag_operations;
    if (pages_moved) *pages_moved = defrag_pages_moved;
    spinlock_unlock(&defrag_lock);
}
