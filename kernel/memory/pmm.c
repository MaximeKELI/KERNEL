#include "memory.h"
#include "string.h"
#include "stdio.h"

/* Physical memory manager using bitmap */
#define PMM_BLOCK_SIZE 4096
#define PMM_BITS_PER_BYTE 8

static u8* pmm_bitmap = NULL;
static u64 pmm_bitmap_size = 0;
static u64 pmm_total_blocks = 0;
static u64 pmm_used_blocks = 0;
static u64 pmm_bitmap_start = 0;

/* Per-frame reference counts. Sized for the largest configuration we boot with
 * (512 MiB / 4 KiB). u16 so a page can be shared by many COW children without
 * overflow. Frame N's count lives at pmm_refcounts[N]. */
#define PMM_MAX_FRAMES (512UL * 1024 * 1024 / PMM_BLOCK_SIZE)
static u16 pmm_refcounts[PMM_MAX_FRAMES];

static inline u64 pmm_block_of(void* phys) {
    return (u64)phys / PMM_BLOCK_SIZE;
}

void pmm_init(u64 mem_size, u64 reserved_start, u64 reserved_size) {
    /* Calculate bitmap size */
    pmm_total_blocks = mem_size / PMM_BLOCK_SIZE;
    pmm_bitmap_size = (pmm_total_blocks + PMM_BITS_PER_BYTE - 1) / PMM_BITS_PER_BYTE;
    
    /* Place bitmap at end of reserved area */
    pmm_bitmap_start = reserved_start + reserved_size;
    pmm_bitmap = (u8*)pmm_bitmap_start;
    
    /* Initialize bitmap: all blocks free */
    memset(pmm_bitmap, 0, pmm_bitmap_size);
    
    /* Mark bitmap area as used */
    u64 bitmap_blocks = (pmm_bitmap_size + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
    u64 bitmap_start_block = pmm_bitmap_start / PMM_BLOCK_SIZE;
    
    for (u64 i = 0; i < bitmap_blocks; i++) {
        u64 block = bitmap_start_block + i;
        pmm_bitmap[block / PMM_BITS_PER_BYTE] |= (1 << (block % PMM_BITS_PER_BYTE));
        pmm_used_blocks++;
    }
    
    /* Mark reserved area as used */
    u64 reserved_blocks = (reserved_size + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
    u64 reserved_start_block = reserved_start / PMM_BLOCK_SIZE;
    
    for (u64 i = 0; i < reserved_blocks; i++) {
        u64 block = reserved_start_block + i;
        pmm_bitmap[block / PMM_BITS_PER_BYTE] |= (1 << (block % PMM_BITS_PER_BYTE));
        pmm_used_blocks++;
    }
    
    printk("PMM: Initialized %u MB, %u blocks, %u used\n",
           (u32)(mem_size / (1024 * 1024)),
           (u32)pmm_total_blocks,
           (u32)pmm_used_blocks);
}

void* pmm_alloc(size_t pages) {
    if (pages == 0) return NULL;
    
    u64 consecutive = 0;
    u64 start_block = 0;
    
    for (u64 i = 0; i < pmm_total_blocks; i++) {
        u64 byte_idx = i / PMM_BITS_PER_BYTE;
        u64 bit_idx = i % PMM_BITS_PER_BYTE;
        
        if (!(pmm_bitmap[byte_idx] & (1 << bit_idx))) {
            if (consecutive == 0) {
                start_block = i;
            }
            consecutive++;
            
            if (consecutive >= pages) {
                /* Mark blocks as used */
                for (u64 j = 0; j < pages; j++) {
                    u64 block = start_block + j;
                    u64 byte = block / PMM_BITS_PER_BYTE;
                    u64 bit = block % PMM_BITS_PER_BYTE;
                    pmm_bitmap[byte] |= (1 << bit);
                    pmm_used_blocks++;
                    if (block < PMM_MAX_FRAMES) {
                        pmm_refcounts[block] = 1;
                    }
                }
                return (void*)(start_block * PMM_BLOCK_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }
    
    return NULL; /* Out of memory */
}

void pmm_free(void* addr, size_t pages) {
    if (!addr || pages == 0) return;
    
    u64 start_block = (u64)addr / PMM_BLOCK_SIZE;
    
    for (u64 i = 0; i < pages; i++) {
        u64 block = start_block + i;
        if (block >= pmm_total_blocks) continue;
        
        u64 byte = block / PMM_BITS_PER_BYTE;
        u64 bit = block % PMM_BITS_PER_BYTE;
        
        if (pmm_bitmap[byte] & (1 << bit)) {
            pmm_bitmap[byte] &= ~(1 << bit);
            pmm_used_blocks--;
            if (block < PMM_MAX_FRAMES) {
                pmm_refcounts[block] = 0;
            }
        }
    }
}

void pmm_ref(void* phys_page) {
    u64 block = pmm_block_of(phys_page);
    if (block >= pmm_total_blocks || block >= PMM_MAX_FRAMES) {
        return;
    }
    /* A frame handed out by pmm_alloc has count 1; guard the degenerate case of
     * sharing a frame that was never counted (e.g. identity/boot memory). */
    if (pmm_refcounts[block] == 0) {
        pmm_refcounts[block] = 1;
    }
    pmm_refcounts[block]++;
}

unsigned pmm_unref(void* phys_page) {
    u64 block = pmm_block_of(phys_page);
    if (block >= pmm_total_blocks || block >= PMM_MAX_FRAMES) {
        return 0;
    }
    if (pmm_refcounts[block] <= 1) {
        pmm_free(phys_page, 1);   /* clears the bit and zeroes the count */
        return 0;
    }
    pmm_refcounts[block]--;
    return pmm_refcounts[block];
}

unsigned pmm_refcount(void* phys_page) {
    u64 block = pmm_block_of(phys_page);
    if (block >= pmm_total_blocks || block >= PMM_MAX_FRAMES) {
        return 0;
    }
    return pmm_refcounts[block];
}

size_t pmm_get_free_pages(void) {
    return pmm_total_blocks - pmm_used_blocks;
}

size_t pmm_get_total_pages(void) {
    return pmm_total_blocks;
}
