#include "cache.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_BUFFERS 256
#define MAX_PAGES 512
#define BUFFER_SIZE 4096

static buffer_head_t buffers[MAX_BUFFERS];
static page_cache_t pages[MAX_PAGES];
static buffer_head_t* buffer_lru_head = NULL;
static buffer_head_t* buffer_lru_tail = NULL;
static page_cache_t* page_lru_head = NULL;
static page_cache_t* page_lru_tail = NULL;
static spinlock_t cache_lock = SPINLOCK_INIT;

void cache_init(void) {
    memset(buffers, 0, sizeof(buffers));
    memset(pages, 0, sizeof(pages));
    DEBUG_INFO("Cache system initialized");
}

buffer_head_t* buffer_get(u64 block_num, u32 size) {
    spinlock_lock(&cache_lock);
    
    /* Search for existing buffer */
    for (u32 i = 0; i < MAX_BUFFERS; i++) {
        if (buffers[i].valid && buffers[i].block_num == block_num && buffers[i].size == size) {
            buffers[i].refcount++;
            spinlock_unlock(&cache_lock);
            return &buffers[i];
        }
    }
    
    /* Allocate new buffer */
    for (u32 i = 0; i < MAX_BUFFERS; i++) {
        if (!buffers[i].valid) {
            buffer_head_t* bh = &buffers[i];
            bh->block_num = block_num;
            bh->size = size;
            bh->data = kmalloc(size);
            if (!bh->data) {
                spinlock_unlock(&cache_lock);
                return NULL;
            }
            bh->valid = true;
            bh->dirty = false;
            bh->refcount = 1;
            spinlock_init(&bh->lock);
            
            /* Add to LRU */
            if (!buffer_lru_head) {
                buffer_lru_head = buffer_lru_tail = bh;
            } else {
                bh->next = buffer_lru_head;
                buffer_lru_head->prev = bh;
                buffer_lru_head = bh;
            }
            
            spinlock_unlock(&cache_lock);
            return bh;
        }
    }
    
    /* Evict LRU buffer */
    if (buffer_lru_tail) {
        buffer_head_t* bh = buffer_lru_tail;
        if (bh->dirty) {
            buffer_sync(bh);
        }
        kfree(bh->data);
        memset(bh, 0, sizeof(buffer_head_t));
        
        /* Remove from LRU */
        if (bh->prev) {
            bh->prev->next = NULL;
            buffer_lru_tail = bh->prev;
        } else {
            buffer_lru_head = buffer_lru_tail = NULL;
        }
        
        /* Reuse */
        bh->block_num = block_num;
        bh->size = size;
        bh->data = kmalloc(size);
        if (bh->data) {
            bh->valid = true;
            bh->refcount = 1;
            spinlock_init(&bh->lock);
            
            if (!buffer_lru_head) {
                buffer_lru_head = buffer_lru_tail = bh;
            } else {
                bh->next = buffer_lru_head;
                buffer_lru_head->prev = bh;
                buffer_lru_head = bh;
            }
        }
        
        spinlock_unlock(&cache_lock);
        return bh->data ? bh : NULL;
    }
    
    spinlock_unlock(&cache_lock);
    return NULL;
}

void buffer_put(buffer_head_t* bh) {
    if (!bh) return;
    
    spinlock_lock(&cache_lock);
    if (bh->refcount > 0) {
        bh->refcount--;
    }
    spinlock_unlock(&cache_lock);
}

void buffer_mark_dirty(buffer_head_t* bh) {
    if (!bh) return;
    spinlock_lock(&bh->lock);
    bh->dirty = true;
    spinlock_unlock(&bh->lock);
}

int buffer_sync(buffer_head_t* bh) {
    if (!bh || !bh->dirty) return 0;
    
    /* Would write to disk here */
    (void)bh;
    return 0;
}

page_cache_t* page_cache_get(u64 ino, u64 offset) {
    spinlock_lock(&cache_lock);
    
    /* Search for existing page */
    for (u32 i = 0; i < MAX_PAGES; i++) {
        if (pages[i].page && pages[i].ino == ino && pages[i].offset == offset) {
            pages[i].refcount++;
            spinlock_unlock(&cache_lock);
            return &pages[i];
        }
    }
    
    /* Allocate new page */
    for (u32 i = 0; i < MAX_PAGES; i++) {
        if (!pages[i].page) {
            page_cache_t* page = &pages[i];
            page->ino = ino;
            page->offset = offset;
            page->page = vmm_alloc_pages(1);
            if (!page->page) {
                spinlock_unlock(&cache_lock);
                return NULL;
            }
            page->dirty = false;
            page->refcount = 1;
            spinlock_init(&page->lock);
            
            /* Add to LRU */
            if (!page_lru_head) {
                page_lru_head = page_lru_tail = page;
            } else {
                page->next = page_lru_head;
                page_lru_head->prev = page;
                page_lru_head = page;
            }
            
            spinlock_unlock(&cache_lock);
            return page;
        }
    }
    
    spinlock_unlock(&cache_lock);
    return NULL;
}

void page_cache_put(page_cache_t* page) {
    if (!page) return;
    
    spinlock_lock(&cache_lock);
    if (page->refcount > 0) {
        page->refcount--;
    }
    spinlock_unlock(&cache_lock);
}

void page_cache_mark_dirty(page_cache_t* page) {
    if (!page) return;
    spinlock_lock(&page->lock);
    page->dirty = true;
    spinlock_unlock(&page->lock);
}

int page_cache_sync(page_cache_t* page) {
    if (!page || !page->dirty) return 0;
    
    /* Would write to disk here */
    (void)page;
    return 0;
}

void cache_sync_all(void) {
    spinlock_lock(&cache_lock);
    
    for (u32 i = 0; i < MAX_BUFFERS; i++) {
        if (buffers[i].dirty) {
            buffer_sync(&buffers[i]);
        }
    }
    
    for (u32 i = 0; i < MAX_PAGES; i++) {
        if (pages[i].dirty) {
            page_cache_sync(&pages[i]);
        }
    }
    
    spinlock_unlock(&cache_lock);
}
