#include "compression.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Filesystem Compression */
static bool compression_enabled = false;
static spinlock_t compression_lock = SPINLOCK_INIT;

void fs_compression_init(void) {
    compression_enabled = true;
    printk("[Filesystem Compression] Initialized\n");
}

/* TODO: Implement Filesystem Compression functionality */
