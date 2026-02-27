#include "bfq.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* BFQ I/O Scheduler */
static bool bfq_enabled = false;
static spinlock_t bfq_lock = SPINLOCK_INIT;

void bfq_init(void) {
    bfq_enabled = true;
    printk("[BFQ I/O Scheduler] Initialized\n");
}

/* TODO: Implement BFQ I/O Scheduler functionality */
