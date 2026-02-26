#ifndef BLOCK_H
#define BLOCK_H

#include "types.h"

/* Block I/O request */
typedef struct bio {
    u64 sector;
    u32 count;
    void* buffer;
    u32 flags;
    void (*end_io)(struct bio* bio);
    void* private_data;
    struct bio* next;
} bio_t;

/* Block device */
typedef struct block_device {
    u32 major;
    u32 minor;
    u64 size;
    u32 block_size;
    char name[32];
    void* private_data;
    struct block_device* next;
} block_device_t;

/* Request queue */
typedef struct request_queue {
    bio_t* requests;
    spinlock_t lock;
    void* private_data;
} request_queue_t;

/* Initialize block layer */
void block_init(void);

/* Register block device */
int register_blkdev(u32 major, const char* name, void* ops);

/* Submit bio */
int submit_bio(block_device_t* bdev, bio_t* bio);

/* Process request queue */
void blk_process_queue(request_queue_t* queue);

#endif /* BLOCK_H */
