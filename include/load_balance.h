#ifndef LOAD_BALANCE_H
#define LOAD_BALANCE_H

#include "types.h"
#include "process.h"

/* Initialize load balancing */
void load_balance_init(void);

/* Update CPU load */
void load_balance_update_cpu(u32 cpu_id);

/* Find idle CPU */
u32 load_balance_find_idle_cpu(void);

/* Migrate task to target CPU */
bool load_balance_migrate_task(process_t* proc, u32 target_cpu);

/* Load balance tick (called periodically) */
void load_balance_tick(void);

/* Enable/disable load balancing */
void load_balance_enable(void);
void load_balance_disable(void);
bool load_balance_is_enabled(void);

/* Get statistics */
u64 load_balance_get_operations(void);
void load_balance_get_cpu_load(u32 cpu_id, u64* load, u64* nr_running);

#endif /* LOAD_BALANCE_H */
