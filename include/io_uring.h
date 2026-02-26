#ifndef IO_URING_H
#define IO_URING_H

#include "types.h"

/* IO_URING opcodes */
#define IORING_OP_NOP          0
#define IORING_OP_READV        1
#define IORING_OP_WRITEV       2
#define IORING_OP_FSYNC        3
#define IORING_OP_READ_FIXED   4
#define IORING_OP_WRITE_FIXED  5

/* IO_URING flags */
#define IORING_SETUP_IOPOLL    (1 << 0)
#define IORING_SETUP_SQPOLL    (1 << 1)
#define IORING_SETUP_SQ_AFF    (1 << 2)

/* Submission queue entry */
typedef struct io_uring_sqe {
    u8 opcode;
    u8 flags;
    u16 ioprio;
    i32 fd;
    u64 off;
    u64 addr;
    u32 len;
    u64 user_data;
    u64 __pad2[3];
} io_uring_sqe_t;

/* Completion queue entry */
typedef struct io_uring_cqe {
    u64 user_data;
    i32 res;
    u32 flags;
} io_uring_cqe_t;

/* IO_URING instance */
typedef struct io_uring {
    u32 sq_entries;
    u32 cq_entries;
    io_uring_sqe_t* sq_ring;
    io_uring_cqe_t* cq_ring;
    u32 sq_head;
    u32 sq_tail;
    u32 cq_head;
    u32 cq_tail;
    void* sq_array;
    void* cq_array;
} io_uring_t;

/* Initialize IO_URING */
void io_uring_init(void);

/* Setup IO_URING */
io_uring_t* io_uring_setup(u32 entries, u32 flags);

/* Submit entries */
int io_uring_submit(io_uring_t* ring);

/* Wait for completions */
int io_uring_wait_cqe(io_uring_t* ring, io_uring_cqe_t** cqe);

/* Get completion */
void io_uring_cqe_seen(io_uring_t* ring, io_uring_cqe_t* cqe);

#endif /* IO_URING_H */
