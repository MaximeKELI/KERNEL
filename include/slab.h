#ifndef SLAB_H
#define SLAB_H

#include "types.h"
#include "spinlock.h"

/* SLAB cache */
typedef struct slab_cache {
    char name[32];
    size_t object_size;
    size_t objects_per_slab;
    void* slabs;
    u32 active_objects;
    u32 total_objects;
    spinlock_t lock;
    struct slab_cache* next;
} slab_cache_t;

/* Initialize SLAB allocator */
void slab_init(void);

/* Create SLAB cache */
slab_cache_t* kmem_cache_create(const char* name, size_t size);

/* Allocate from cache */
void* kmem_cache_alloc(slab_cache_t* cache);

/* Free to cache */
void kmem_cache_free(slab_cache_t* cache, void* obj);

/* Destroy cache */
void kmem_cache_destroy(slab_cache_t* cache);

/* Reclaim free slabs (returns approximate page count) */
u64 slab_reclaim_pages(u64 max_pages);

/* Per-CPU cache operations */
void* kmem_cache_alloc_cpu(slab_cache_t* cache, u32 cpu_id);
void kmem_cache_free_cpu(slab_cache_t* cache, void* obj, u32 cpu_id);

#endif /* SLAB_H */
