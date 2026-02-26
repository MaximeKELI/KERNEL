#include "cpu_isolation.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"
#include "smp.h"

#define MAX_CPUS 64

typedef struct cpu_isolation_state {
    u32 mode;
    u32 flags;
    bool isolated;
    u64 allowed_processes;
    spinlock_t lock;
} cpu_isolation_state_t;

static cpu_isolation_state_t cpu_states[MAX_CPUS];
static spinlock_t isolation_global_lock = SPINLOCK_INIT;

void cpu_isolation_init(void) {
    for (u32 i = 0; i < MAX_CPUS; i++) {
        cpu_states[i].mode = CPU_ISOLATION_NONE;
        cpu_states[i].flags = 0;
        cpu_states[i].isolated = false;
        cpu_states[i].allowed_processes = 0;
        spinlock_init(&cpu_states[i].lock);
    }
    DEBUG_INFO("%s", "CPU isolation initialized");
}

int cpu_isolate(u32 cpu, u32 mode, u32 flags) {
    if (cpu >= MAX_CPUS) {
        DEBUG_ERROR("Invalid CPU ID: %u", cpu);
        return -1;
    }
    
    if (mode > CPU_ISOLATION_HOUSEKEEPING) {
        DEBUG_ERROR("Invalid isolation mode: %u", mode);
        return -1;
    }
    
    spinlock_lock(&isolation_global_lock);
    spinlock_lock(&cpu_states[cpu].lock);
    
    if (cpu_states[cpu].isolated) {
        spinlock_unlock(&cpu_states[cpu].lock);
        spinlock_unlock(&isolation_global_lock);
        DEBUG_WARN("CPU %u already isolated", cpu);
        return -1;
    }
    
    cpu_states[cpu].mode = mode;
    cpu_states[cpu].flags = flags;
    cpu_states[cpu].isolated = true;
    
    /* Would disable interrupts, RCU, migration on this CPU */
    if (flags & CPU_ISOLATION_NO_TICK) {
        /* Disable tick */
    }
    if (flags & CPU_ISOLATION_NO_RCU) {
        /* Disable RCU */
    }
    if (flags & CPU_ISOLATION_NO_MIGRATE) {
        /* Disable process migration */
    }
    
    spinlock_unlock(&cpu_states[cpu].lock);
    spinlock_unlock(&isolation_global_lock);
    
    DEBUG_INFO("CPU %u isolated: mode=%u, flags=0x%x", cpu, mode, flags);
    return 0;
}

int cpu_unisolate(u32 cpu) {
    if (cpu >= MAX_CPUS) {
        return -1;
    }
    
    spinlock_lock(&isolation_global_lock);
    spinlock_lock(&cpu_states[cpu].lock);
    
    if (!cpu_states[cpu].isolated) {
        spinlock_unlock(&cpu_states[cpu].lock);
        spinlock_unlock(&isolation_global_lock);
        return 0;
    }
    
    cpu_states[cpu].isolated = false;
    cpu_states[cpu].mode = CPU_ISOLATION_NONE;
    cpu_states[cpu].flags = 0;
    
    /* Would re-enable interrupts, RCU, migration */
    
    spinlock_unlock(&cpu_states[cpu].lock);
    spinlock_unlock(&isolation_global_lock);
    
    DEBUG_INFO("CPU %u unisolated", cpu);
    return 0;
}

bool cpu_is_isolated(u32 cpu) {
    if (cpu >= MAX_CPUS) {
        return false;
    }
    
    spinlock_lock(&cpu_states[cpu].lock);
    bool isolated = cpu_states[cpu].isolated;
    spinlock_unlock(&cpu_states[cpu].lock);
    
    return isolated;
}

u32 cpu_get_isolation_mode(u32 cpu) {
    if (cpu >= MAX_CPUS) {
        return CPU_ISOLATION_NONE;
    }
    
    spinlock_lock(&cpu_states[cpu].lock);
    u32 mode = cpu_states[cpu].mode;
    spinlock_unlock(&cpu_states[cpu].lock);
    
    return mode;
}

int cpu_set_affinity(u64 pid, u64 cpu_mask) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            /* Would set CPU affinity mask */
            /* Check if any isolated CPUs are in mask */
            for (u32 i = 0; i < MAX_CPUS; i++) {
                if ((cpu_mask & (1ULL << i)) && cpu_is_isolated(i)) {
                    DEBUG_WARN("Process %u affinity includes isolated CPU %u", pid, i);
                }
            }
            
            DEBUG_INFO("CPU affinity set: pid=%u, mask=0x%llx", pid, (unsigned long long)cpu_mask);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

u64 cpu_get_affinity(u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            /* Would return CPU affinity mask */
            return 0xFFFFFFFFFFFFFFFFULL; /* All CPUs */
        }
        proc = proc->next;
    }
    
    return 0;
}
