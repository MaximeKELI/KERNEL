#include "io_uring.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

static io_uring_t* io_uring_instances = NULL;
static spinlock_t io_uring_lock = SPINLOCK_INIT;

void io_uring_init(void) {
    DEBUG_INFO("IO_URING (async I/O) system initialized");
}

io_uring_t* io_uring_setup(u32 entries, u32 flags) {
    (void)flags;
    VALIDATE_RANGE_NULL(entries, 1, 4096);
    
    io_uring_t* ring = (io_uring_t*)kzalloc(sizeof(io_uring_t));
    if (!ring) {
        DEBUG_ERROR("Failed to allocate IO_URING");
        return NULL;
    }
    
    ring->sq_entries = entries;
    ring->cq_entries = entries * 2;
    
    /* Allocate submission queue */
    size_t sq_size = entries * sizeof(io_uring_sqe_t);
    ring->sq_ring = (io_uring_sqe_t*)vmm_alloc_pages((sq_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    /* Allocate completion queue */
    size_t cq_size = ring->cq_entries * sizeof(io_uring_cqe_t);
    ring->cq_ring = (io_uring_cqe_t*)vmm_alloc_pages((cq_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!ring->sq_ring || !ring->cq_ring) {
        if (ring->sq_ring) vmm_free_pages(ring->sq_ring, (sq_size + PAGE_SIZE - 1) / PAGE_SIZE);
        if (ring->cq_ring) vmm_free_pages(ring->cq_ring, (cq_size + PAGE_SIZE - 1) / PAGE_SIZE);
        kfree(ring);
        return NULL;
    }
    
    ring->sq_head = ring->sq_tail = 0;
    ring->cq_head = ring->cq_tail = 0;
    
    spinlock_lock(&io_uring_lock);
    ring->sq_array = (void*)((u64)ring + sizeof(io_uring_t));
    ring->cq_array = (void*)((u64)ring->sq_array + entries * sizeof(u32));
    ring->sq_array = NULL; /* Would be mapped */
    ring->cq_array = NULL;
    spinlock_unlock(&io_uring_lock);
    
    DEBUG_INFO("IO_URING setup: entries=%u", entries);
    return ring;
}

int io_uring_submit(io_uring_t* ring) {
    VALIDATE_PTR_RET(ring, -1);
    
    /* Would submit entries to kernel */
    u32 submitted = ring->sq_tail - ring->sq_head;
    ring->sq_head = ring->sq_tail;
    
    return submitted;
}

int io_uring_wait_cqe(io_uring_t* ring, io_uring_cqe_t** cqe) {
    VALIDATE_PTR_RET(ring, -1);
    VALIDATE_PTR_RET(cqe, -1);
    
    /* Wait for completion */
    while (ring->cq_tail == ring->cq_head) {
        /* Would wait here */
        __asm__ __volatile__("pause");
    }
    
    *cqe = &ring->cq_ring[ring->cq_head & (ring->cq_entries - 1)];
    return 0;
}

void io_uring_cqe_seen(io_uring_t* ring, io_uring_cqe_t* cqe) {
    (void)cqe;
    VALIDATE_PTR_VOID(ring);
    
    ring->cq_head++;
}
