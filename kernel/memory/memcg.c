#include "memcg.h"
#include "memory.h"
#include "cgroup.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"
#include "string.h"

#define MAX_MEMCG 256

typedef struct memcg {
    u32 id;
    u64 limit;          /* Memory limit in bytes */
    u64 usage;          /* Current usage in bytes */
    u64 soft_limit;     /* Soft limit */
    u64 swap_limit;     /* Swap limit */
    u64 fail_count;     /* OOM failures */
    bool active;
    spinlock_t lock;
} memcg_t;

static memcg_t memcgs[MAX_MEMCG];
static u32 memcg_count = 0;
static spinlock_t memcg_global_lock = SPINLOCK_INIT;

void memcg_init(void) {
    memset(memcgs, 0, sizeof(memcgs));
    memcg_count = 0;
    printk("[MemCG] Memory Cgroup v2 initialized\n");
}

u32 memcg_create(void) {
    spinlock_lock(&memcg_global_lock);
    
    if (memcg_count >= MAX_MEMCG) {
        spinlock_unlock(&memcg_global_lock);
        return 0;
    }
    
    u32 id = memcg_count + 1;
    memcg_t* cg = &memcgs[memcg_count++];
    
    cg->id = id;
    cg->limit = 0; /* No limit by default */
    cg->usage = 0;
    cg->soft_limit = 0;
    cg->swap_limit = 0;
    cg->fail_count = 0;
    cg->active = true;
    spinlock_init(&cg->lock);
    
    spinlock_unlock(&memcg_global_lock);
    
    return id;
}

void memcg_destroy(u32 id) {
    if (id == 0 || id > memcg_count) {
        return;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    cg->active = false;
    cg->usage = 0;
    spinlock_unlock(&cg->lock);
}

bool memcg_set_limit(u32 id, u64 limit) {
    if (id == 0 || id > memcg_count) {
        return false;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    if (!cg->active) {
        spinlock_unlock(&cg->lock);
        return false;
    }
    cg->limit = limit;
    spinlock_unlock(&cg->lock);
    
    return true;
}

u64 memcg_get_limit(u32 id) {
    if (id == 0 || id > memcg_count) {
        return 0;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    u64 limit = cg->limit;
    spinlock_unlock(&cg->lock);
    
    return limit;
}

bool memcg_charge(u32 id, u64 size) {
    if (id == 0 || id > memcg_count) {
        return false;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    if (!cg->active) {
        spinlock_unlock(&cg->lock);
        return false;
    }
    
    /* Check limit */
    if (cg->limit > 0 && (cg->usage + size) > cg->limit) {
        cg->fail_count++;
        spinlock_unlock(&cg->lock);
        return false;
    }
    
    cg->usage += size;
    spinlock_unlock(&cg->lock);
    
    return true;
}

void memcg_uncharge(u32 id, u64 size) {
    if (id == 0 || id > memcg_count) {
        return;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    if (cg->usage >= size) {
        cg->usage -= size;
    } else {
        cg->usage = 0;
    }
    spinlock_unlock(&cg->lock);
}

u64 memcg_get_usage(u32 id) {
    if (id == 0 || id > memcg_count) {
        return 0;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    u64 usage = cg->usage;
    spinlock_unlock(&cg->lock);
    
    return usage;
}

void memcg_get_stats(u32 id, memcg_stats_t* stats) {
    if (!stats || id == 0 || id > memcg_count) {
        return;
    }
    
    memcg_t* cg = &memcgs[id - 1];
    
    spinlock_lock(&cg->lock);
    stats->usage = cg->usage;
    stats->limit = cg->limit;
    stats->soft_limit = cg->soft_limit;
    stats->swap_limit = cg->swap_limit;
    stats->fail_count = cg->fail_count;
    spinlock_unlock(&cg->lock);
}
