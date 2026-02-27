#include "multipath.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Network Multipath */
static bool multipath_enabled = false;
static spinlock_t multipath_lock = SPINLOCK_INIT;

void multipath_init(void) {
    multipath_enabled = true;
    printk("[Network Multipath] Initialized\n");
}

/* TODO: Implement Network Multipath functionality */
