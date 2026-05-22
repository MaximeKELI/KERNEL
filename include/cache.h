#ifndef CACHE_H
#define CACHE_H

#include "types.h"
#include "spinlock.h"

/* Buffer cache for block devices */
typedef struct buffer_head {
    u64 block_num;
    void* data;
    u32 size;
    bool dirty;
    bool valid;
    u32 refcount;
    struct buffer_head* next;
    struct buffer_head* prev;
    spinlock_t lock;
} buffer_head_t;

/* Page cache for file data */
typedef struct page_cache {
    u64 ino;
    u64 offset;
    void* page;
    bool dirty;
    u32 refcount;
    struct page_cache* next;
    struct page_cache* prev;
    spinlock_t lock;
} page_cache_t;

/* Initialize cache system */
void cache_init(void);

/* Buffer cache operations */
buffer_head_t* buffer_get(u64 block_num, u32 size);
void buffer_put(buffer_head_t* bh);
void buffer_mark_dirty(buffer_head_t* bh);
int buffer_sync(buffer_head_t* bh);

/* Page cache operations */
page_cache_t* page_cache_get(u64 ino, u64 offset);
void page_cache_put(page_cache_t* page);
void page_cache_mark_dirty(page_cache_t* page);
int page_cache_sync(page_cache_t* page);

/* Sync all caches */
void cache_sync_all(void);

#endif /* CACHE_H */
