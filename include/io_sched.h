#ifndef IO_SCHED_H
#define IO_SCHED_H

#include "types.h"
#include "block.h"

/* I/O scheduler types */
#define IOSCHED_NOOP     0
#define IOSCHED_DEADLINE 1
#define IOSCHED_CFQ      2

/* I/O scheduler */
typedef struct io_scheduler {
    u32 type;
    char name[32];
    int (*add_request)(struct io_scheduler* iosched, bio_t* bio);
    bio_t* (*dispatch_request)(struct io_scheduler* iosched);
    void (*init)(struct io_scheduler* iosched);
    void* private_data;
} io_scheduler_t;

/* Initialize I/O schedulers */
void io_sched_init(void);

/* Set I/O scheduler */
int io_sched_set(request_queue_t* queue, u32 type);

/* Get I/O scheduler */
io_scheduler_t* io_sched_get(request_queue_t* queue);

#endif /* IO_SCHED_H */
