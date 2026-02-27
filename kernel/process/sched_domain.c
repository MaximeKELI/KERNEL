#include "sched_domain.h"
#include "smp.h"
#include "numa.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"
#include "string.h"

#define MAX_DOMAINS 16
#define MAX_CPUS_PER_DOMAIN 64

/* Scheduler domain types */
#define SD_CPU     0  /* CPU domain */
#define SD_CACHE   1  /* Cache domain */
#define SD_NUMA    2  /* NUMA domain */

typedef struct sched_domain {
    u32 id;
    u32 type;
    u32 cpu_count;
    u32 cpus[MAX_CPUS_PER_DOMAIN];
    u32 parent_id;
    u32 child_count;
    u32 children[MAX_DOMAINS];
    u64 load;           /* Domain load */
    bool active;
    spinlock_t lock;
} sched_domain_t;

static sched_domain_t domains[MAX_DOMAINS];
static u32 domain_count = 0;
static spinlock_t domain_lock = SPINLOCK_INIT;

void sched_domain_init(void) {
    memset(domains, 0, sizeof(domains));
    domain_count = 0;
    
    /* Create CPU domains */
    u32 cpu_count = smp_get_cpu_count();
    for (u32 i = 0; i < cpu_count && domain_count < MAX_DOMAINS; i++) {
        sched_domain_t* domain = &domains[domain_count];
        domain->id = domain_count;
        domain->type = SD_CPU;
        domain->cpu_count = 1;
        domain->cpus[0] = i;
        domain->parent_id = 0;
        domain->child_count = 0;
        domain->load = 0;
        domain->active = true;
        spinlock_init(&domain->lock);
        domain_count++;
    }
    
    printk("[Sched Domain] Initialized %u domains\n", domain_count);
}

u32 sched_domain_create(u32 type, u32* cpus, u32 cpu_count) {
    if (!cpus || cpu_count == 0 || cpu_count > MAX_CPUS_PER_DOMAIN) {
        return 0;
    }
    
    spinlock_lock(&domain_lock);
    
    if (domain_count >= MAX_DOMAINS) {
        spinlock_unlock(&domain_lock);
        return 0;
    }
    
    sched_domain_t* domain = &domains[domain_count];
    domain->id = domain_count++;
    domain->type = type;
    domain->cpu_count = cpu_count;
    memcpy(domain->cpus, cpus, cpu_count * sizeof(u32));
    domain->parent_id = 0;
    domain->child_count = 0;
    domain->load = 0;
    domain->active = true;
    spinlock_init(&domain->lock);
    
    spinlock_unlock(&domain_lock);
    
    return domain->id;
}

void sched_domain_destroy(u32 id) {
    if (id >= domain_count) {
        return;
    }
    
    sched_domain_t* domain = &domains[id];
    
    spinlock_lock(&domain->lock);
    domain->active = false;
    domain->cpu_count = 0;
    spinlock_unlock(&domain->lock);
}

u32 sched_domain_find_for_cpu(u32 cpu_id) {
    spinlock_lock(&domain_lock);
    
    for (u32 i = 0; i < domain_count; i++) {
        sched_domain_t* domain = &domains[i];
        if (!domain->active) continue;
        
        for (u32 j = 0; j < domain->cpu_count; j++) {
            if (domain->cpus[j] == cpu_id) {
                spinlock_unlock(&domain_lock);
                return domain->id;
            }
        }
    }
    
    spinlock_unlock(&domain_lock);
    return 0;
}

void sched_domain_update_load(u32 id, u64 load) {
    if (id >= domain_count) {
        return;
    }
    
    sched_domain_t* domain = &domains[id];
    
    spinlock_lock(&domain->lock);
    domain->load = load;
    spinlock_unlock(&domain->lock);
}

u64 sched_domain_get_load(u32 id) {
    if (id >= domain_count) {
        return 0;
    }
    
    sched_domain_t* domain = &domains[id];
    
    spinlock_lock(&domain->lock);
    u64 load = domain->load;
    spinlock_unlock(&domain->lock);
    
    return load;
}

void sched_domain_set_parent(u32 id, u32 parent_id) {
    if (id >= domain_count || parent_id >= domain_count) {
        return;
    }
    
    sched_domain_t* domain = &domains[id];
    sched_domain_t* parent = &domains[parent_id];
    
    spinlock_lock(&domain->lock);
    domain->parent_id = parent_id;
    spinlock_unlock(&domain->lock);
    
    spinlock_lock(&parent->lock);
    if (parent->child_count < MAX_DOMAINS) {
        parent->children[parent->child_count++] = id;
    }
    spinlock_unlock(&parent->lock);
}

u32 sched_domain_find_best_cpu(u32 domain_id) {
    if (domain_id >= domain_count) {
        return 0;
    }
    
    sched_domain_t* domain = &domains[domain_id];
    
    spinlock_lock(&domain->lock);
    
    /* Find CPU with lowest load in domain */
    u32 best_cpu = domain->cpus[0];
    u64 min_load = UINT64_MAX;
    
    for (u32 i = 0; i < domain->cpu_count; i++) {
        /* TODO: Get actual CPU load */
        u64 cpu_load = 0;
        if (cpu_load < min_load) {
            min_load = cpu_load;
            best_cpu = domain->cpus[i];
        }
    }
    
    spinlock_unlock(&domain->lock);
    
    return best_cpu;
}
