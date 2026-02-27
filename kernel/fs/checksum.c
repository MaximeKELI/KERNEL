#include "checksum.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Filesystem Checksumming */
static bool checksum_enabled = false;
static spinlock_t checksum_lock = SPINLOCK_INIT;

void fs_checksum_init(void) {
    checksum_enabled = true;
    printk("[Filesystem Checksumming] Initialized\n");
}

/* TODO: Implement Filesystem Checksumming functionality */
