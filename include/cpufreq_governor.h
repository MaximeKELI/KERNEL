#ifndef CPUFREQ_GOVERNOR_H
#define CPUFREQ_GOVERNOR_H

#include "types.h"

/* CPU frequency governors */
#define CPUFREQ_GOVERNOR_PERFORMANCE 0
#define CPUFREQ_GOVERNOR_POWERSAVE    1
#define CPUFREQ_GOVERNOR_ONDEMAND    2
#define CPUFREQ_GOVERNOR_CONSERVATIVE 3
#define CPUFREQ_GOVERNOR_SCHEDUTIL   4

/* Governor operations */
typedef struct cpufreq_governor {
    char name[32];
    u32 type;
    int (*init)(u32 cpu);
    int (*exit)(u32 cpu);
    int (*start)(u32 cpu);
    int (*stop)(u32 cpu);
    int (*limits)(u32 cpu, u32 min, u32 max);
} cpufreq_governor_t;

/* Set governor */
int cpufreq_set_governor(u32 cpu, u32 governor);

/* Get governor */
u32 cpufreq_get_governor(u32 cpu);

/* Register custom governor */
int cpufreq_register_governor(cpufreq_governor_t* governor);

/* Unregister governor */
int cpufreq_unregister_governor(const char* name);

#endif /* CPUFREQ_GOVERNOR_H */
