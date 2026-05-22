#include "block.h"
#include "blk_mq.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

/* Global I/O statistics for AI monitoring */
u64 global_io_read_bytes = 0;
u64 global_io_write_bytes = 0;
u64 global_io_read_ops = 0;
u64 global_io_write_ops = 0;

static block_device_t* block_devices = NULL;
static spinlock_t block_lock = SPINLOCK_INIT;

void block_init(void) {
    blk_mq_init();
    DEBUG_INFO("Block layer initialized");
}

int register_blkdev(u32 major, const char* name, void* ops) {
    block_device_t* bdev = (block_device_t*)kzalloc(sizeof(block_device_t));
    if (!bdev) {
        DEBUG_ERROR("Failed to allocate block device");
        return -1;
    }
    
    bdev->major = major;
    bdev->minor = 0;
    strncpy(bdev->name, name, sizeof(bdev->name) - 1);
    bdev->private_data = ops;
    
    spinlock_lock(&block_lock);
    bdev->next = block_devices;
    block_devices = bdev;
    spinlock_unlock(&block_lock);
    
    DEBUG_INFO("Block device registered: %s (major %u)", name, major);
    return 0;
}

int submit_bio(block_device_t* bdev, bio_t* bio) {
    if (!bdev || !bio) return -1;
    
    request_queue_t* queue = (request_queue_t*)bdev->private_data;
    if (!queue) return -1;
    
    /* Update I/O statistics */
    /* Use flags to determine read/write (bit 0 = read, bit 1 = write) */
    u32 sector_size = 512; /* Standard sector size */
    u64 io_size = (u64)bio->count * sector_size;
    
    if (bio->flags & 0x1) { /* Read flag */
        global_io_read_bytes += io_size;
        global_io_read_ops++;
    } else if (bio->flags & 0x2) { /* Write flag */
        global_io_write_bytes += io_size;
        global_io_write_ops++;
    }
    
    u32 qid = (u32)((u64)bio->sector % blk_mq_queue_count());
    if (blk_mq_submit(bio, qid) == 0) {
        blk_mq_dispatch(qid);
        return 0;
    }

    spinlock_lock(&queue->lock);
    bio->next = queue->requests;
    queue->requests = bio;
    spinlock_unlock(&queue->lock);
    blk_process_queue(queue);
    return 0;
}

void blk_process_queue(request_queue_t* queue) {
    if (!queue) return;
    
    spinlock_lock(&queue->lock);
    
    bio_t* bio = queue->requests;
    while (bio) {
        bio_t* next = bio->next;
        
        /* Would execute I/O here */
        if (bio->end_io) {
            bio->end_io(bio);
        }
        
        bio = next;
    }
    
    queue->requests = NULL;
    spinlock_unlock(&queue->lock);
}
