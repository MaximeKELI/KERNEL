#include "bcache.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Block Cache */
static bool bcache_enabled = false;
static spinlock_t bcache_lock = SPINLOCK_INIT;

void bcache_init(void) {
    bcache_enabled = true;
    printk("[Block Cache] Initialized\n");
}

/* TODO: Implement Block Cache functionality */
