#include "slab.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

#define SLAB_SIZE PAGE_SIZE
#define MAX_CACHES 64

static slab_cache_t* cache_list = NULL;
static spinlock_t slab_lock = SPINLOCK_INIT;

/* SLAB structure */
typedef struct slab {
    void* objects;
    u32 free_count;
    u32* free_list;
    struct slab* next;
} slab_t;

void slab_init(void) {
    DEBUG_INFO("%s", "SLAB allocator initialized");
}

slab_cache_t* kmem_cache_create(const char* name, size_t size) {
    slab_cache_t* cache = (slab_cache_t*)kzalloc(sizeof(slab_cache_t));
    if (!cache) {
        DEBUG_ERROR("%s", "Failed to allocate SLAB cache");
        return NULL;
    }
    
    strncpy(cache->name, name, sizeof(cache->name) - 1);
    cache->object_size = ALIGN_UP(size, 8);
    cache->objects_per_slab = (SLAB_SIZE - sizeof(slab_t)) / cache->object_size;
    cache->active_objects = 0;
    cache->total_objects = 0;
    spinlock_init(&cache->lock);
    
    spinlock_lock(&slab_lock);
    cache->next = cache_list;
    cache_list = cache;
    spinlock_unlock(&slab_lock);
    
    DEBUG_INFO("SLAB cache created: %s (size=%u, objects/slab=%u)",
               name, (u32)size, (u32)cache->objects_per_slab);
    
    return cache;
}

void* kmem_cache_alloc(slab_cache_t* cache) {
    if (!cache) return NULL;
    
    spinlock_lock(&cache->lock);
    
    /* Find slab with free objects */
    slab_t* slab = (slab_t*)cache->slabs;
    while (slab) {
        if (slab->free_count > 0) {
            u32 idx = slab->free_list[--slab->free_count];
            void* obj = (u8*)slab->objects + idx * cache->object_size;
            cache->active_objects++;
            spinlock_unlock(&cache->lock);
            return obj;
        }
        slab = slab->next;
    }
    
    /* Allocate new slab */
    slab = (slab_t*)vmm_alloc_pages(1);
    if (!slab) {
        spinlock_unlock(&cache->lock);
        return NULL;
    }
    
    slab->objects = (u8*)slab + sizeof(slab_t);
    slab->free_count = cache->objects_per_slab;
    slab->free_list = (u32*)((u8*)slab->objects + cache->objects_per_slab * cache->object_size);
    
    /* Initialize free list */
    for (u32 i = 0; i < cache->objects_per_slab; i++) {
        slab->free_list[i] = i;
    }
    
    slab->next = (slab_t*)cache->slabs;
    cache->slabs = slab;
    cache->total_objects += cache->objects_per_slab;
    
    u32 idx = slab->free_list[--slab->free_count];
    void* obj = (u8*)slab->objects + idx * cache->object_size;
    cache->active_objects++;
    
    spinlock_unlock(&cache->lock);
    return obj;
}

void kmem_cache_free(slab_cache_t* cache, void* obj) {
    if (!cache || !obj) return;
    
    spinlock_lock(&cache->lock);
    
    /* Find slab containing object */
    slab_t* slab = (slab_t*)cache->slabs;
    while (slab) {
        if ((u8*)obj >= (u8*)slab->objects && (u8*)obj < (u8*)slab->free_list) {
            u32 idx = ((u8*)obj - (u8*)slab->objects) / cache->object_size;
            slab->free_list[slab->free_count++] = idx;
            cache->active_objects--;
            spinlock_unlock(&cache->lock);
            return;
        }
        slab = slab->next;
    }
    
    spinlock_unlock(&cache->lock);
    DEBUG_WARN("%s", "Object not found in SLAB cache");
}

void kmem_cache_destroy(slab_cache_t* cache) {
    if (!cache) return;
    
    spinlock_lock(&slab_lock);
    
    slab_cache_t** prev = &cache_list;
    slab_cache_t* c = cache_list;
    while (c) {
        if (c == cache) {
            *prev = c->next;
            break;
        }
        prev = &c->next;
        c = c->next;
    }
    
    spinlock_unlock(&slab_lock);
    
    /* Free all slabs */
    slab_t* slab = (slab_t*)cache->slabs;
    while (slab) {
        slab_t* next = slab->next;
        vmm_free_pages(slab, 1);
        slab = next;
    }
    
    kfree(cache);
}

void* kmem_cache_alloc_cpu(slab_cache_t* cache, u32 cpu_id) {
    (void)cpu_id;
    return kmem_cache_alloc(cache);
}

void kmem_cache_free_cpu(slab_cache_t* cache, void* obj, u32 cpu_id) {
    (void)cpu_id;
    kmem_cache_free(cache, obj);
}

u64 slab_reclaim_pages(u64 max_pages) {
    u64 reclaimed = 0;

    if (max_pages == 0) {
        return 0;
    }

    spinlock_lock(&slab_lock);

    for (slab_cache_t* cache = cache_list; cache && reclaimed < max_pages; cache = cache->next) {
        spinlock_lock(&cache->lock);

        slab_t** prev = (slab_t**)&cache->slabs;
        slab_t* slab = (slab_t*)cache->slabs;

        while (slab && reclaimed < max_pages) {
            if (slab->free_count != cache->objects_per_slab) {
                prev = &slab->next;
                slab = slab->next;
                continue;
            }

            *prev = slab->next;
            vmm_free_pages(slab, 1);
            cache->total_objects -= cache->objects_per_slab;
            reclaimed++;
            slab = *prev;
        }

        spinlock_unlock(&cache->lock);
    }

    spinlock_unlock(&slab_lock);
    return reclaimed;
}
