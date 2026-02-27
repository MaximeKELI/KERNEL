#ifndef SCHED_DOMAIN_H
#define SCHED_DOMAIN_H

#include "types.h"

/* Scheduler domain types */
#define SCHED_DOMAIN_CPU   0
#define SCHED_DOMAIN_CACHE 1
#define SCHED_DOMAIN_NUMA  2

/* Initialize scheduler domains */
void sched_domain_init(void);

/* Create/destroy scheduler domain */
u32 sched_domain_create(u32 type, u32* cpus, u32 cpu_count);
void sched_domain_destroy(u32 id);

/* Find domain for CPU */
u32 sched_domain_find_for_cpu(u32 cpu_id);

/* Update/get domain load */
void sched_domain_update_load(u32 id, u64 load);
u64 sched_domain_get_load(u32 id);

/* Set domain parent (hierarchy) */
void sched_domain_set_parent(u32 id, u32 parent_id);

/* Find best CPU in domain */
u32 sched_domain_find_best_cpu(u32 domain_id);

#endif /* SCHED_DOMAIN_H */
