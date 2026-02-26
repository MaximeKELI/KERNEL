#include "oom_killer.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"
#include "scheduler.h"
#include "pmm.h"

static u32 oom_mode = OOM_MODE_ENABLED;
static spinlock_t oom_lock = SPINLOCK_INIT;

void oom_killer_init(void) {
    oom_mode = OOM_MODE_ENABLED;
    DEBUG_INFO("%s", "OOM killer initialized");
}

bool oom_check(void) {
    if (oom_mode == OOM_MODE_DISABLED) {
        return false;
    }
    
    size_t free_pages = pmm_get_free_pages();
    size_t total_pages = pmm_get_total_pages();
    
    if (total_pages == 0) {
        return false;
    }
    
    u32 free_percent = (u32)(free_pages * 100 / total_pages);
    
    /* OOM condition: less than 5% free memory */
    return free_percent < 5;
}

u32 oom_calculate_score(process_t* proc) {
    if (!proc) {
        return 0;
    }
    
    /* Calculate score based on memory usage, CPU time, etc. */
    u64 memory_score = proc->stack_size / PAGE_SIZE;
    u64 cpu_score = proc->runtime / 1000; /* Normalize */
    u64 children_score = 0;
    
    /* Count children memory */
    extern process_t* process_list;
    process_t* p = process_list;
    while (p) {
        if (p->parent_pid == proc->pid) {
            children_score += p->stack_size / PAGE_SIZE;
        }
        p = p->next;
    }
    
    /* Weighted score */
    u32 score = (u32)(memory_score * 3 + cpu_score + children_score * 2);
    
    return score;
}

int oom_kill_process(void) {
    if (oom_mode == OOM_MODE_DISABLED) {
        return -1;
    }
    
    if (oom_mode == OOM_MODE_PANIC) {
        panic("Out of memory - OOM panic mode");
        return -1;
    }
    
    spinlock_lock(&oom_lock);
    
    extern process_t* process_list;
    process_t* victim = NULL;
    u32 max_score = 0;
    
    /* Find process with highest OOM score */
    process_t* p = process_list;
    while (p) {
        if (p->state != PROCESS_DEAD && p->state != PROCESS_ZOMBIE) {
            u32 score = oom_calculate_score(p);
            if (score > max_score) {
                max_score = score;
                victim = p;
            }
        }
        p = p->next;
    }
    
    if (victim) {
        DEBUG_ERROR("OOM killer: killing process %u (score=%u)", victim->pid, max_score);
        process_destroy(victim);
        spinlock_unlock(&oom_lock);
        return 0;
    }
    
    spinlock_unlock(&oom_lock);
    return -1;
}

int oom_set_mode(u32 mode) {
    if (mode > OOM_MODE_PANIC) {
        return -1;
    }
    
    spinlock_lock(&oom_lock);
    oom_mode = mode;
    spinlock_unlock(&oom_lock);
    
    DEBUG_INFO("OOM killer mode set: %u", mode);
    return 0;
}

u32 oom_get_mode(void) {
    return oom_mode;
}
