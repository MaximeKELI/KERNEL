#include "sched_tune.h"
#include "scheduler.h"
#include "process.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

#define MAX_TUNE_ENTRIES 64

typedef struct {
    u64 pid;
    i32 boost;
    bool active;
} sched_tune_entry_t;

static bool sched_tune_enabled = false;
static sched_tune_entry_t entries[MAX_TUNE_ENTRIES];
static spinlock_t sched_tune_lock = SPINLOCK_INIT;

static sched_tune_entry_t* find_entry(u64 pid, bool create) {
    sched_tune_entry_t* free_slot = NULL;

    for (u32 i = 0; i < MAX_TUNE_ENTRIES; i++) {
        if (entries[i].active && entries[i].pid == pid) {
            return &entries[i];
        }
        if (!entries[i].active && !free_slot) {
            free_slot = &entries[i];
        }
    }

    if (!create || !free_slot) {
        return NULL;
    }

    free_slot->pid = pid;
    free_slot->boost = 0;
    free_slot->active = true;
    return free_slot;
}

void sched_tune_init(void) {
    memset(entries, 0, sizeof(entries));
    sched_tune_enabled = true;
    printk("[Scheduler Tuning] Initialized\n");
}

int sched_tune_set_boost(u64 pid, i32 boost) {
    if (!sched_tune_enabled) {
        return -1;
    }

    spinlock_lock(&sched_tune_lock);
    sched_tune_entry_t* entry = find_entry(pid, true);
    if (!entry) {
        spinlock_unlock(&sched_tune_lock);
        return -1;
    }

    entry->boost = boost;
    spinlock_unlock(&sched_tune_lock);

    if (setpriority(pid, boost) != 0) {
        return -1;
    }
    return 0;
}

int sched_tune_clear_boost(u64 pid) {
    return sched_tune_set_boost(pid, 0);
}

i32 sched_tune_get_boost(u64 pid) {
    i32 boost = 0;

    spinlock_lock(&sched_tune_lock);
    sched_tune_entry_t* entry = find_entry(pid, false);
    if (entry) {
        boost = entry->boost;
    }
    spinlock_unlock(&sched_tune_lock);

    return boost;
}
