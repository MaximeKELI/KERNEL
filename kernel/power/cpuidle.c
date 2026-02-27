#include "cpuidle.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* CPU Idle States */
static bool cpuidle_enabled = false;
static spinlock_t cpuidle_lock = SPINLOCK_INIT;

void cpuidle_init(void) {
    cpuidle_enabled = true;
    printk("[CPU Idle States] Initialized\n");
}

/* TODO: Implement CPU Idle States functionality */
