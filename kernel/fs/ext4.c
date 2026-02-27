#include "ext4.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* EXT4 Journaling Filesystem */
static bool ext4_enabled = false;
static spinlock_t ext4_lock = SPINLOCK_INIT;

void ext4_init(void) {
    ext4_enabled = true;
    printk("[EXT4 Journaling Filesystem] Initialized\n");
}

/* TODO: Implement EXT4 Journaling Filesystem functionality */
