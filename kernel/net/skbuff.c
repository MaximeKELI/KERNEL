#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"
#include "string.h"

#define SKB_POOL_SIZE 1024
#define SKB_HEADROOM 128
#define SKB_TAILROOM 128

/* Socket buffer pool */
static sk_buff_t* skb_pool = NULL;
static u32 skb_pool_count = 0;
static spinlock_t skb_pool_lock = SPINLOCK_INIT;

void skb_init(void) {
    skb_pool = NULL;
    skb_pool_count = 0;
    printk("[SKB] Socket buffer subsystem initialized\n");
}

sk_buff_t* skb_alloc(size_t size) {
    size_t total_size = sizeof(sk_buff_t) + SKB_HEADROOM + size + SKB_TAILROOM;
    size_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    void* data = vmm_alloc_pages(pages);
    if (!data) {
        DEBUG_ERROR("Failed to allocate sk_buff");
        return NULL;
    }
    
    sk_buff_t* skb = (sk_buff_t*)data;
    memset(skb, 0, sizeof(sk_buff_t));
    
    /* head marks the start of the buffer; data sits SKB_HEADROOM bytes later so
     * that skb_push() has room to prepend headers (Linux sk_buff semantics). */
    skb->head = (u8*)data + sizeof(sk_buff_t);
    skb->data = skb->head + SKB_HEADROOM;
    skb->tail = skb->data;
    skb->end = skb->data + size;
    skb->len = 0;
    skb->data_len = 0;
    skb->truesize = total_size;
    skb->users = 1;
    skb->protocol = 0;
    skb->pkt_type = 0;
    skb->dev = NULL;
    skb->next = NULL;
    skb->prev = NULL;
    
    return skb;
}

void skb_free(sk_buff_t* skb) {
    if (!skb) {
        return;
    }
    
    skb->users--;
    if (skb->users > 0) {
        return; /* Still in use */
    }
    
    if (skb->dev) {
        /* Release device reference */
    }
    
    /* Free pages (head points just past the sk_buff header at the base). */
    void* data = (void*)((u64)skb->head - sizeof(sk_buff_t));
    size_t pages = (skb->truesize + PAGE_SIZE - 1) / PAGE_SIZE;
    vmm_free_pages(data, pages);
}

void* skb_put(sk_buff_t* skb, size_t len) {
    if (!skb || (skb->tail + len) > skb->end) {
        return NULL;
    }
    
    void* old_tail = skb->tail;
    skb->tail += len;
    skb->len += len;
    
    return old_tail;
}

void* skb_push(sk_buff_t* skb, size_t len) {
    if (!skb || (skb->data - len) < skb->head) {
        return NULL;
    }
    
    skb->data -= len;
    skb->len += len;
    
    return skb->data;
}

void* skb_pull(sk_buff_t* skb, size_t len) {
    if (!skb || len > skb->len) {
        return NULL;
    }
    
    void* old_data = skb->data;
    skb->data += len;
    skb->len -= len;
    
    return old_data;
}

void skb_reserve(sk_buff_t* skb, size_t len) {
    if (!skb) {
        return;
    }
    
    skb->data += len;
    skb->tail = skb->data;
}

sk_buff_t* skb_clone(sk_buff_t* skb) {
    if (!skb) {
        return NULL;
    }
    
    sk_buff_t* new_skb = skb_alloc(skb->len);
    if (!new_skb) {
        return NULL;
    }
    
    memcpy(new_skb->data, skb->data, skb->len);
    new_skb->len = skb->len;
    new_skb->protocol = skb->protocol;
    new_skb->pkt_type = skb->pkt_type;
    new_skb->dev = skb->dev;
    
    skb->users++;
    
    return new_skb;
}
