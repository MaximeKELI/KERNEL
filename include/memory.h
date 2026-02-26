#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Physical memory manager */
void pmm_init(u64 mem_size, u64 reserved_start, u64 reserved_size);
void* pmm_alloc(size_t pages);
void pmm_free(void* addr, size_t pages);
size_t pmm_get_free_pages(void);
size_t pmm_get_total_pages(void);

/* Virtual memory manager */
void vmm_init(void);
void* vmm_map_page(void* virt, void* phys, u64 flags);
void vmm_unmap_page(void* virt);
void* vmm_alloc_pages(size_t pages);
void vmm_free_pages(void* virt, size_t pages);
bool vmm_is_mapped(void* virt);
void* vmm_get_phys(void* virt);

/* Page flags */
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)
#define PAGE_PWT      (1 << 3)
#define PAGE_PCD      (1 << 4)
#define PAGE_SIZE_2MB (1 << 7)
#define PAGE_SIZE_1GB (1 << 7)
#define PAGE_GLOBAL   (1 << 8)
#define PAGE_NX       (1ULL << 63)

/* Kernel heap */
void heap_init(void);
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);

#endif /* MEMORY_H */
