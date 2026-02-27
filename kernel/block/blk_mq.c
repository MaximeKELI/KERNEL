#include "blk_mq.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Block Multi-Queue */
static bool blk_mq_enabled = false;
static spinlock_t blk_mq_lock = SPINLOCK_INIT;

void blk_mq_init(void) {
    blk_mq_enabled = true;
    printk("[Block Multi-Queue] Initialized\n");
}

/* TODO: Implement Block Multi-Queue functionality */
