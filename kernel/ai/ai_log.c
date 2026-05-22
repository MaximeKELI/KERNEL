#include "ai_log.h"
#include "drivers/timer.h"
#include "ai_policy.h"
#include "spinlock.h"

static ai_log_entry_t ring[AI_LOG_SIZE];
static u32 head = 0;
static u32 count = 0;
static u64 total_decisions = 0;
static spinlock_t log_lock = SPINLOCK_INIT;

void ai_log_init(void) {
    spinlock_lock(&log_lock);
    head = 0;
    count = 0;
    total_decisions = 0;
    memset(ring, 0, sizeof(ring));
    spinlock_unlock(&log_lock);
}

void ai_log_record(u8 action, u32 pid, u32 value) {
    if (action == AI_ACT_NONE) {
        return;
    }

    spinlock_lock(&log_lock);
    ai_log_entry_t* e = &ring[head];
    e->tick = timer_get_ticks();
    e->action = action;
    e->policy = (u8)ai_policy_get_mode();
    e->pid = pid;
    e->value = value;
    head = (head + 1) % AI_LOG_SIZE;
    if (count < AI_LOG_SIZE) {
        count++;
    }
    total_decisions++;
    spinlock_unlock(&log_lock);
}

u32 ai_log_count(void) {
    spinlock_lock(&log_lock);
    u32 c = count;
    spinlock_unlock(&log_lock);
    return c;
}

const ai_log_entry_t* ai_log_entry(u32 index) {
    spinlock_lock(&log_lock);
    if (index >= count) {
        spinlock_unlock(&log_lock);
        return NULL;
    }
    u32 start = (head + AI_LOG_SIZE - count) % AI_LOG_SIZE;
    u32 idx = (start + index) % AI_LOG_SIZE;
    const ai_log_entry_t* e = &ring[idx];
    spinlock_unlock(&log_lock);
    return e;
}

void ai_log_clear(void) {
    spinlock_lock(&log_lock);
    head = 0;
    count = 0;
    memset(ring, 0, sizeof(ring));
    spinlock_unlock(&log_lock);
}

u64 ai_log_total_decisions(void) {
    spinlock_lock(&log_lock);
    u64 t = total_decisions;
    spinlock_unlock(&log_lock);
    return t;
}
