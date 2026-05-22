#include "sched_stats.h"
#include "scheduler.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"
#include "string.h"

static bool sched_stats_enabled = false;
static sched_stats_t stats;
static spinlock_t sched_stats_lock = SPINLOCK_INIT;

void sched_stats_init(void) {
    memset(&stats, 0, sizeof(stats));
    sched_stats_enabled = true;
    printk("[Scheduler Statistics] Initialized\n");
}

void sched_stats_record_switch(void) {
    if (!sched_stats_enabled) {
        return;
    }
    spinlock_lock(&sched_stats_lock);
    stats.context_switches++;
    spinlock_unlock(&sched_stats_lock);
}

void sched_stats_record_schedule(void) {
    if (!sched_stats_enabled) {
        return;
    }
    spinlock_lock(&sched_stats_lock);
    stats.schedule_calls++;
    stats.nr_running = scheduler_get_running_count(0);
    spinlock_unlock(&sched_stats_lock);
}

void sched_stats_get(sched_stats_t* out) {
    if (!out) {
        return;
    }
    spinlock_lock(&sched_stats_lock);
    *out = stats;
    spinlock_unlock(&sched_stats_lock);
}
