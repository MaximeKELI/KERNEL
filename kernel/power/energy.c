#include "energy.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

static cpufreq_policy_t cpufreq_policies[16] = {0};
static power_domain_t* power_domains = NULL;
static spinlock_t energy_lock = SPINLOCK_INIT;

void energy_init(void) {
    /* Initialize default policies */
    for (u32 i = 0; i < 16; i++) {
        cpufreq_policies[i].cpu = i;
        cpufreq_policies[i].min_freq = 800;  /* 800 MHz */
        cpufreq_policies[i].max_freq = 3000; /* 3 GHz */
        cpufreq_policies[i].cur_freq = 2000; /* 2 GHz */
        cpufreq_policies[i].governor = 0;    /* Performance */
    }
    
    DEBUG_INFO("Energy management (CPU frequency scaling, power domains) initialized");
}

int cpufreq_set_policy(u32 cpu, cpufreq_policy_t* policy) {
    if (cpu >= 16 || !policy) return -1;
    
    spinlock_lock(&energy_lock);
    cpufreq_policies[cpu] = *policy;
    spinlock_unlock(&energy_lock);
    
    DEBUG_INFO("CPU frequency policy set: CPU %u, freq=%u-%u MHz",
               cpu, policy->min_freq, policy->max_freq);
    return 0;
}

int cpufreq_get_policy(u32 cpu, cpufreq_policy_t* policy) {
    if (cpu >= 16 || !policy) return -1;
    
    spinlock_lock(&energy_lock);
    *policy = cpufreq_policies[cpu];
    spinlock_unlock(&energy_lock);
    
    return 0;
}

int cpufreq_set_frequency(u32 cpu, u32 freq) {
    if (cpu >= 16) return -1;
    
    spinlock_lock(&energy_lock);
    if (freq >= cpufreq_policies[cpu].min_freq && 
        freq <= cpufreq_policies[cpu].max_freq) {
        cpufreq_policies[cpu].cur_freq = freq;
        /* Would set actual CPU frequency */
        spinlock_unlock(&energy_lock);
        DEBUG_INFO("CPU frequency set: CPU %u, freq=%u MHz", cpu, freq);
        return 0;
    }
    spinlock_unlock(&energy_lock);
    return -1;
}

int power_domain_register(power_domain_t* domain) {
    if (!domain || !domain->name) return -1;
    
    spinlock_lock(&energy_lock);
    domain->next = power_domains;
    power_domains = domain;
    spinlock_unlock(&energy_lock);
    
    DEBUG_INFO("Power domain registered: %s", domain->name);
    return 0;
}

int power_domain_enable(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&energy_lock);
    power_domain_t* domain = power_domains;
    while (domain) {
        if (strcmp(domain->name, name) == 0) {
            domain->enabled = true;
            spinlock_unlock(&energy_lock);
            DEBUG_INFO("Power domain enabled: %s", name);
            return 0;
        }
        domain = domain->next;
    }
    spinlock_unlock(&energy_lock);
    return -1;
}

int power_domain_disable(const char* name) {
    if (!name) return -1;
    
    spinlock_lock(&energy_lock);
    power_domain_t* domain = power_domains;
    while (domain) {
        if (strcmp(domain->name, name) == 0) {
            domain->enabled = false;
            spinlock_unlock(&energy_lock);
            DEBUG_INFO("Power domain disabled: %s", name);
            return 0;
        }
        domain = domain->next;
    }
    spinlock_unlock(&energy_lock);
    return -1;
}

void cpu_idle_enter(void) {
    /* Enter CPU idle state */
    asm volatile("hlt");
}

void cpu_idle_exit(void) {
    /* Exit CPU idle state */
}
