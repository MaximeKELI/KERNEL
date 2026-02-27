#include "tc.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Traffic Control */
static bool tc_enabled = false;
static spinlock_t tc_lock = SPINLOCK_INIT;

void tc_init(void) {
    tc_enabled = true;
    printk("[Traffic Control] Initialized\n");
}

/* TODO: Implement Traffic Control functionality */
