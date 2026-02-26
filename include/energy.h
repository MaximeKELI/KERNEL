#ifndef ENERGY_H
#define ENERGY_H

#include "types.h"

/* CPU frequency scaling */
typedef struct cpufreq_policy {
    u32 cpu;
    u32 min_freq;
    u32 max_freq;
    u32 cur_freq;
    u32 governor;
} cpufreq_policy_t;

/* Power domains */
typedef struct power_domain {
    char name[32];
    bool enabled;
    u32 power_state;
    struct power_domain* parent;
    struct power_domain* next;
} power_domain_t;

/* Initialize energy management */
void energy_init(void);

/* CPU frequency scaling */
int cpufreq_set_policy(u32 cpu, cpufreq_policy_t* policy);
int cpufreq_get_policy(u32 cpu, cpufreq_policy_t* policy);
int cpufreq_set_frequency(u32 cpu, u32 freq);

/* Power domains */
int power_domain_register(power_domain_t* domain);
int power_domain_enable(const char* name);
int power_domain_disable(const char* name);

/* CPU idle */
void cpu_idle_enter(void);
void cpu_idle_exit(void);

#endif /* ENERGY_H */
