#include "packet_sched.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Packet Scheduler */
static bool packet_sched_enabled = false;
static spinlock_t packet_sched_lock = SPINLOCK_INIT;

void packet_sched_init(void) {
    packet_sched_enabled = true;
    printk("[Packet Scheduler] Initialized\n");
}

/* TODO: Implement Packet Scheduler functionality */
