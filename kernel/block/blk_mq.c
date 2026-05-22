#include "blk_mq.h"
#include "block.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

#define BLK_MQ_MAX_QUEUES 8
#define BLK_MQ_QUEUE_DEPTH 128

typedef struct blk_mq_ctx {
    u32 queue_id;
    bio_t* pending;
    u64 submitted;
    u64 completed;
    spinlock_t lock;
} blk_mq_ctx_t;

static blk_mq_ctx_t mq_queues[BLK_MQ_MAX_QUEUES];
static u32 mq_queue_count = 0;
static bool blk_mq_enabled = false;
static spinlock_t blk_mq_lock = SPINLOCK_INIT;

void blk_mq_init(void) {
    if (blk_mq_enabled) {
        return;
    }
    memset(mq_queues, 0, sizeof(mq_queues));
    mq_queue_count = 1;
    for (u32 i = 0; i < BLK_MQ_MAX_QUEUES; i++) {
        mq_queues[i].queue_id = i;
        spinlock_init(&mq_queues[i].lock);
    }
    blk_mq_enabled = true;
    printk("[blk-mq] %u hardware queues\n", mq_queue_count);
}

u32 blk_mq_queue_count(void) {
    return mq_queue_count;
}

int blk_mq_submit(bio_t* bio, u32 queue_id) {
    if (!blk_mq_enabled || !bio) {
        return -1;
    }
    if (queue_id >= mq_queue_count) {
        queue_id = 0;
    }

    blk_mq_ctx_t* ctx = &mq_queues[queue_id];
    spinlock_lock(&ctx->lock);
    bio->next = ctx->pending;
    ctx->pending = bio;
    ctx->submitted++;
    spinlock_unlock(&ctx->lock);
    return 0;
}

u32 blk_mq_dispatch(u32 queue_id) {
    u32 done = 0;

    if (!blk_mq_enabled || queue_id >= mq_queue_count) {
        return 0;
    }

    blk_mq_ctx_t* ctx = &mq_queues[queue_id];
    spinlock_lock(&ctx->lock);

    bio_t* bio = ctx->pending;
    while (bio) {
        bio_t* next = bio->next;
        if (bio->end_io) {
            bio->end_io(bio);
        }
        ctx->completed++;
        done++;
        bio = next;
    }
    ctx->pending = NULL;

    spinlock_unlock(&ctx->lock);
    return done;
}

void blk_mq_get_stats(u32 queue_id, u64* submitted, u64* completed) {
    if (queue_id >= mq_queue_count) {
        return;
    }
    blk_mq_ctx_t* ctx = &mq_queues[queue_id];
    spinlock_lock(&ctx->lock);
    if (submitted) {
        *submitted = ctx->submitted;
    }
    if (completed) {
        *completed = ctx->completed;
    }
    spinlock_unlock(&ctx->lock);
}
