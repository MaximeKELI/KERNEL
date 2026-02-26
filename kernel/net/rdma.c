#include "rdma.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

static rdma_qp_t* rdma_qps = NULL;
static u32 next_qp_num = 1;
static spinlock_t rdma_lock = SPINLOCK_INIT;

void rdma_init(void) {
    DEBUG_INFO("RDMA (Remote Direct Memory Access) initialized");
}

rdma_qp_t* rdma_create_qp(u32 port) {
    (void)port;
    
    rdma_qp_t* qp = (rdma_qp_t*)kzalloc(sizeof(rdma_qp_t));
    if (!qp) {
        DEBUG_ERROR("Failed to allocate RDMA QP");
        return NULL;
    }
    
    qp->send_queue = vmm_alloc_pages(1);
    qp->recv_queue = vmm_alloc_pages(1);
    
    if (!qp->send_queue || !qp->recv_queue) {
        if (qp->send_queue) vmm_free_pages(qp->send_queue, 1);
        if (qp->recv_queue) vmm_free_pages(qp->recv_queue, 1);
        kfree(qp);
        return NULL;
    }
    
    spinlock_lock(&rdma_lock);
    qp->qp_num = next_qp_num++;
    qp->state = 1; /* Ready */
    qp->next = rdma_qps;
    rdma_qps = qp;
    spinlock_unlock(&rdma_lock);
    
    DEBUG_INFO("RDMA QP created: qp_num=%u", qp->qp_num);
    return qp;
}

int rdma_post_send(rdma_qp_t* qp, void* buffer, size_t length, u64 wr_id) {
    VALIDATE_PTR_RET(qp, -1);
    VALIDATE_PTR_RET(buffer, -1);
    VALIDATE_SIZE(length);
    
    /* Would post to send queue */
    DEBUG_INFO("RDMA send posted: qp=%u, len=%u", qp->qp_num, (u32)length);
    return 0;
}

int rdma_post_recv(rdma_qp_t* qp, void* buffer, size_t length, u64 wr_id) {
    VALIDATE_PTR_RET(qp, -1);
    VALIDATE_PTR_RET(buffer, -1);
    VALIDATE_SIZE(length);
    
    /* Would post to recv queue */
    DEBUG_INFO("RDMA recv posted: qp=%u, len=%u", qp->qp_num, (u32)length);
    return 0;
}

int rdma_poll_cq(rdma_qp_t* qp, rdma_completion_t* comp) {
    VALIDATE_PTR_RET(qp, -1);
    VALIDATE_PTR_RET(comp, -1);
    
    /* Would poll completion queue */
    comp->status = 0;
    comp->byte_len = 0;
    return 0;
}
