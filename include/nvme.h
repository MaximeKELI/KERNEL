#ifndef NVME_H
#define NVME_H

#include "types.h"

/* NVMe queue */
typedef struct nvme_queue {
    u16 qid;
    void* sq_base;
    void* cq_base;
    u32 sq_size;
    u32 cq_size;
    u16 sq_tail;
    u16 cq_head;
    struct nvme_queue* next;
} nvme_queue_t;

/* NVMe command */
typedef struct nvme_command {
    u32 opcode;
    u32 nsid;
    u64 prp1;
    u64 prp2;
    u32 cdw10;
    u32 cdw11;
    u32 cdw12;
    u32 cdw13;
    u32 cdw14;
    u32 cdw15;
} nvme_command_t;

/* Initialize NVMe */
void nvme_init(void);

/* Identify controller */
int nvme_identify_controller(void* buffer);

/* Create I/O queue */
nvme_queue_t* nvme_create_io_queue(u16 qid, u32 qsize);

/* Submit command */
int nvme_submit_command(nvme_queue_t* queue, nvme_command_t* cmd);

/* Read data */
int nvme_read(nvme_queue_t* queue, u32 nsid, u64 lba, u32 count, void* buffer);

/* Write data */
int nvme_write(nvme_queue_t* queue, u32 nsid, u64 lba, u32 count, const void* buffer);

#endif /* NVME_H */
