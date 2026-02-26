#include "cpufreq_governor.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "energy.h"

#define MAX_GOVERNORS 16

static cpufreq_governor_t governors[MAX_GOVERNORS];
static u32 governor_per_cpu[64] = {0};
static spinlock_t governor_lock = SPINLOCK_INIT;

static int performance_init(u32 cpu) {
    (void)cpu;
    return 0;
}

static int performance_start(u32 cpu) {
    /* Set to maximum frequency */
    cpufreq_policy_t policy;
    if (cpufreq_get_policy(cpu, &policy) == 0) {
        cpufreq_set_frequency(cpu, policy.max_freq);
    }
    return 0;
}

static int powersave_init(u32 cpu) {
    (void)cpu;
    return 0;
}

static int powersave_start(u32 cpu) {
    /* Set to minimum frequency */
    cpufreq_policy_t policy;
    if (cpufreq_get_policy(cpu, &policy) == 0) {
        cpufreq_set_frequency(cpu, policy.min_freq);
    }
    return 0;
}

static int ondemand_init(u32 cpu) {
    (void)cpu;
    return 0;
}

static int ondemand_start(u32 cpu) {
    /* Would monitor CPU load and adjust frequency */
    return 0;
}

void cpufreq_governor_init(void) {
    /* Register built-in governors */
    strncpy(governors[CPUFREQ_GOVERNOR_PERFORMANCE].name, "performance", 31);
    governors[CPUFREQ_GOVERNOR_PERFORMANCE].type = CPUFREQ_GOVERNOR_PERFORMANCE;
    governors[CPUFREQ_GOVERNOR_PERFORMANCE].init = performance_init;
    governors[CPUFREQ_GOVERNOR_PERFORMANCE].start = performance_start;
    
    strncpy(governors[CPUFREQ_GOVERNOR_POWERSAVE].name, "powersave", 31);
    governors[CPUFREQ_GOVERNOR_POWERSAVE].type = CPUFREQ_GOVERNOR_POWERSAVE;
    governors[CPUFREQ_GOVERNOR_POWERSAVE].init = powersave_init;
    governors[CPUFREQ_GOVERNOR_POWERSAVE].start = powersave_start;
    
    strncpy(governors[CPUFREQ_GOVERNOR_ONDEMAND].name, "ondemand", 31);
    governors[CPUFREQ_GOVERNOR_ONDEMAND].type = CPUFREQ_GOVERNOR_ONDEMAND;
    governors[CPUFREQ_GOVERNOR_ONDEMAND].init = ondemand_init;
    governors[CPUFREQ_GOVERNOR_ONDEMAND].start = ondemand_start;
    
    DEBUG_INFO("%s", "CPU frequency governors initialized");
}

int cpufreq_set_governor(u32 cpu, u32 governor) {
    if (cpu >= 64 || governor >= MAX_GOVERNORS) {
        return -1;
    }
    
    spinlock_lock(&governor_lock);
    
    if (governor_per_cpu[cpu] != governor) {
        /* Stop old governor */
        if (governors[governor_per_cpu[cpu]].stop) {
            governors[governor_per_cpu[cpu]].stop(cpu);
        }
        
        /* Start new governor */
        if (governors[governor].init) {
            governors[governor].init(cpu);
        }
        if (governors[governor].start) {
            governors[governor].start(cpu);
        }
        
        governor_per_cpu[cpu] = governor;
    }
    
    spinlock_unlock(&governor_lock);
    
    DEBUG_INFO("CPU frequency governor set: cpu=%u, governor=%u", cpu, governor);
    return 0;
}

u32 cpufreq_get_governor(u32 cpu) {
    if (cpu >= 64) {
        return 0;
    }
    
    return governor_per_cpu[cpu];
}

int cpufreq_register_governor(cpufreq_governor_t* governor) {
    VALIDATE_PTR_RET(governor, -1);
    VALIDATE_STRING(governor->name, 32);
    
    spinlock_lock(&governor_lock);
    
    for (u32 i = 4; i < MAX_GOVERNORS; i++) {
        if (governors[i].name[0] == '\0') {
            governors[i] = *governor;
            spinlock_unlock(&governor_lock);
            DEBUG_INFO("Custom governor registered: %s", governor->name);
            return 0;
        }
    }
    
    spinlock_unlock(&governor_lock);
    DEBUG_ERROR("%s", "Maximum governors reached");
    return -1;
}

int cpufreq_unregister_governor(const char* name) {
    VALIDATE_STRING(name, 32);
    
    spinlock_lock(&governor_lock);
    
    for (u32 i = 4; i < MAX_GOVERNORS; i++) {
        if (strcmp(governors[i].name, name) == 0) {
            memset(&governors[i], 0, sizeof(governors[i]));
            spinlock_unlock(&governor_lock);
            DEBUG_INFO("Governor unregistered: %s", name);
            return 0;
        }
    }
    
    spinlock_unlock(&governor_lock);
    return -1;
}
