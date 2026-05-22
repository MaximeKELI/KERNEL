#include "load_balance.h"
#include "scheduler.h"
#include "smp.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"
#include "string.h"
#include "drivers/timer.h"

/* Load balancing */
static bool load_balance_enabled = true;
static u64 balance_operations = 0;
static spinlock_t balance_lock = SPINLOCK_INIT;

/* CPU load tracking */
typedef struct {
    u64 load;           /* Current load */
    u64 nr_running;     /* Number of running tasks */
    u64 last_update;     /* Last update timestamp */
} cpu_load_t;

static cpu_load_t cpu_loads[64]; /* Support up to 64 CPUs */
static u32 cpu_count = 0;

void load_balance_init(void) {
    load_balance_enabled = true;
    balance_operations = 0;
    memset(cpu_loads, 0, sizeof(cpu_loads));
    
    cpu_count = smp_get_cpu_count();
    if (cpu_count > 64) cpu_count = 64;
    
    printk("[Load Balance] Initialized for %u CPUs\n", cpu_count);
}

void load_balance_update_cpu(u32 cpu_id) {
    if (cpu_id >= cpu_count) {
        return;
    }
    
    cpu_load_t* load = &cpu_loads[cpu_id];
    
    spinlock_lock(&balance_lock);
    
    /* Update load based on running tasks */
    load->nr_running = scheduler_get_running_count(cpu_id);
    load->load = load->nr_running * 100; /* Simplified load calculation */
    load->last_update = 0; /* TODO: Use actual timestamp */
    
    spinlock_unlock(&balance_lock);
}

u32 load_balance_find_idle_cpu(void) {
    u32 idle_cpu = 0;
    u64 min_load = UINT64_MAX;
    
    spinlock_lock(&balance_lock);
    
    for (u32 i = 0; i < cpu_count; i++) {
        if (cpu_loads[i].load < min_load) {
            min_load = cpu_loads[i].load;
            idle_cpu = i;
        }
    }
    
    spinlock_unlock(&balance_lock);
    
    return idle_cpu;
}

bool load_balance_migrate_task(process_t* proc, u32 target_cpu) {
    if (!proc || target_cpu >= cpu_count) {
        return false;
    }
    
    /* TODO: Implement actual task migration */
    /* This would involve updating process CPU affinity */
    
    spinlock_lock(&balance_lock);
    balance_operations++;
    spinlock_unlock(&balance_lock);
    
    DEBUG_INFO("Migrating task %u to CPU %u", proc->pid, target_cpu);
    
    return true;
}

void load_balance_tick(void) {
    if (!load_balance_enabled) {
        return;
    }
    
    /* Update all CPU loads */
    for (u32 i = 0; i < cpu_count; i++) {
        load_balance_update_cpu(i);
    }
    
    /* Find CPUs with high load */
    for (u32 i = 0; i < cpu_count; i++) {
        cpu_load_t* load = &cpu_loads[i];
        
        if (load->load > 200) { /* High load threshold */
            /* Find idle CPU */
            u32 idle_cpu = load_balance_find_idle_cpu();
            
            if (idle_cpu != i && cpu_loads[idle_cpu].load < 100) {
                /* TODO: Migrate tasks from CPU i to idle_cpu */
                DEBUG_INFO("Load balance: CPU %u overloaded, migrating to CPU %u", i, idle_cpu);
            }
        }
    }
}

void load_balance_enable(void) {
    spinlock_lock(&balance_lock);
    load_balance_enabled = true;
    spinlock_unlock(&balance_lock);
}

void load_balance_disable(void) {
    spinlock_lock(&balance_lock);
    load_balance_enabled = false;
    spinlock_unlock(&balance_lock);
}

bool load_balance_is_enabled(void) {
    spinlock_lock(&balance_lock);
    bool enabled = load_balance_enabled;
    spinlock_unlock(&balance_lock);
    return enabled;
}

u64 load_balance_get_operations(void) {
    spinlock_lock(&balance_lock);
    u64 ops = balance_operations;
    spinlock_unlock(&balance_lock);
    return ops;
}

void load_balance_get_cpu_load(u32 cpu_id, u64* load, u64* nr_running) {
    if (cpu_id >= cpu_count) {
        return;
    }
    
    cpu_load_t* cpu_load = &cpu_loads[cpu_id];
    
    spinlock_lock(&balance_lock);
    if (load) *load = cpu_load->load;
    if (nr_running) *nr_running = cpu_load->nr_running;
    spinlock_unlock(&balance_lock);
}
