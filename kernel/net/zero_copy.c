#include "zero_copy.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_ZERO_COPY_BUFFERS 1024
#define ZERO_COPY_BUFFER_SIZE (64 * 1024) /* 64KB */

/* Zero-copy buffer */
typedef struct zero_copy_buffer {
    void* pages;
    u64 page_count;
    u64 offset;
    u64 size;
    u32 users;
    bool pinned;
    struct zero_copy_buffer* next;
} zero_copy_buffer_t;

static zero_copy_buffer_t* zc_pool = NULL;
static u32 zc_pool_count = 0;
static spinlock_t zc_lock = SPINLOCK_INIT;

void zero_copy_init(void) {
    zc_pool = NULL;
    zc_pool_count = 0;
    printk("[Zero-Copy] Zero-copy networking initialized\n");
}

zero_copy_buffer_t* zero_copy_alloc(size_t size) {
    if (size == 0 || size > ZERO_COPY_BUFFER_SIZE) {
        return NULL;
    }
    
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Allocate physically contiguous pages */
    void* phys = pmm_alloc(pages);
    if (!phys) {
        return NULL;
    }
    
    /* Map to kernel virtual address */
    void* virt = vmm_map_page(phys, phys, PAGE_PRESENT | PAGE_WRITABLE);
    if (!virt) {
        pmm_free(phys, pages);
        return NULL;
    }
    
    zero_copy_buffer_t* zcb = (zero_copy_buffer_t*)kzalloc(sizeof(zero_copy_buffer_t));
    if (!zcb) {
        vmm_unmap_page(virt);
        pmm_free(phys, pages);
        return NULL;
    }
    
    zcb->pages = virt;
    zcb->page_count = pages;
    zcb->offset = 0;
    zcb->size = size;
    zcb->users = 1;
    zcb->pinned = true;
    
    spinlock_lock(&zc_lock);
    zcb->next = zc_pool;
    zc_pool = zcb;
    zc_pool_count++;
    spinlock_unlock(&zc_lock);
    
    DEBUG_INFO("Zero-copy buffer allocated: %u pages", (u32)pages);
    return zcb;
}

void zero_copy_free(zero_copy_buffer_t* zcb) {
    if (!zcb) {
        return;
    }
    
    zcb->users--;
    if (zcb->users > 0) {
        return; /* Still in use */
    }
    
    spinlock_lock(&zc_lock);
    
    /* Remove from pool */
    if (zc_pool == zcb) {
        zc_pool = zcb->next;
    } else {
        zero_copy_buffer_t* p = zc_pool;
        while (p && p->next != zcb) {
            p = p->next;
        }
        if (p) {
            p->next = zcb->next;
        }
    }
    
    zc_pool_count--;
    spinlock_unlock(&zc_lock);
    
    /* Unmap and free */
    void* phys = vmm_get_phys(zcb->pages);
    if (phys) {
        vmm_unmap_page(zcb->pages);
        pmm_free(phys, zcb->page_count);
    }
    
    kfree(zcb);
}

void* zero_copy_get_data(zero_copy_buffer_t* zcb) {
    if (!zcb) {
        return NULL;
    }
    
    return (u8*)zcb->pages + zcb->offset;
}

int zero_copy_send(socket_t* sock, zero_copy_buffer_t* zcb, size_t len) {
    if (!sock || !zcb || len == 0) {
        return -1;
    }
    
    /* Send directly from zero-copy buffer without copying */
    void* data = zero_copy_get_data(zcb);
    
    /* TODO: Send via network stack without copying */
    /* This would involve DMA directly from the buffer */
    
    zcb->users++;
    return 0;
}

int zero_copy_recv(socket_t* sock, zero_copy_buffer_t* zcb, size_t len) {
    if (!sock || !zcb || len == 0) {
        return -1;
    }
    
    /* Receive directly into zero-copy buffer */
    void* data = zero_copy_get_data(zcb);
    
    /* TODO: Receive via DMA directly into buffer */
    
    return 0;
}

u32 zero_copy_get_pool_count(void) {
    spinlock_lock(&zc_lock);
    u32 count = zc_pool_count;
    spinlock_unlock(&zc_lock);
    return count;
}
