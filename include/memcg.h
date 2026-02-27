#ifndef MEMCG_H
#define MEMCG_H

#include "types.h"

/* Memory cgroup statistics */
typedef struct {
    u64 usage;
    u64 limit;
    u64 soft_limit;
    u64 swap_limit;
    u64 fail_count;
} memcg_stats_t;

/* Initialize memory cgroup system */
void memcg_init(void);

/* Create/destroy memory cgroup */
u32 memcg_create(void);
void memcg_destroy(u32 id);

/* Set/get memory limit */
bool memcg_set_limit(u32 id, u64 limit);
u64 memcg_get_limit(u32 id);

/* Charge/uncharge memory */
bool memcg_charge(u32 id, u64 size);
void memcg_uncharge(u32 id, u64 size);

/* Get current usage */
u64 memcg_get_usage(u32 id);

/* Get statistics */
void memcg_get_stats(u32 id, memcg_stats_t* stats);

#endif /* MEMCG_H */
