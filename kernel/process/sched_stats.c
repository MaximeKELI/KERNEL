#include "sched_stats.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Scheduler Statistics */
static bool sched_stats_enabled = false;
static spinlock_t sched_stats_lock = SPINLOCK_INIT;

void sched_stats_init(void) {
    sched_stats_enabled = true;
    printk("[Scheduler Statistics] Initialized\n");
}

/* TODO: Implement Scheduler Statistics functionality */
