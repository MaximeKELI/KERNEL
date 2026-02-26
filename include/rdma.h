#ifndef RDMA_H
#define RDMA_H

#include "types.h"

/* RDMA operations */
#define RDMA_OP_SEND    0
#define RDMA_OP_RECV    1
#define RDMA_OP_WRITE   2
#define RDMA_OP_READ    3

/* RDMA queue pair */
typedef struct rdma_qp {
    u32 qp_num;
    void* send_queue;
    void* recv_queue;
    u32 state;
    struct rdma_qp* next;
} rdma_qp_t;

/* RDMA completion */
typedef struct rdma_completion {
    u64 wr_id;
    u32 status;
    u32 byte_len;
} rdma_completion_t;

/* Initialize RDMA */
void rdma_init(void);

/* Create queue pair */
rdma_qp_t* rdma_create_qp(u32 port);

/* Post send */
int rdma_post_send(rdma_qp_t* qp, void* buffer, size_t length, u64 wr_id);

/* Post receive */
int rdma_post_recv(rdma_qp_t* qp, void* buffer, size_t length, u64 wr_id);

/* Poll completion */
int rdma_poll_cq(rdma_qp_t* qp, rdma_completion_t* comp);

#endif /* RDMA_H */
