#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "process.h"

/* Scheduler types */
#define SCHED_NORMAL 0
#define SCHED_FIFO   1
#define SCHED_RR     2
#define SCHED_CFS    3

/* Process priority (nice value) */
#define PRIO_MIN -20
#define PRIO_MAX  19
#define PRIO_DEFAULT 0

/* Scheduler statistics */
typedef struct {
    u64 total_switches;
    u64 total_runtime;
    u64 idle_time;
} scheduler_stats_t;

/* Initialize advanced scheduler */
void scheduler_init(void);

/* Schedule next process (CFS-like) */
void schedule(void);

/* Set process priority */
int setpriority(u64 pid, int priority);

/* Get process priority */
int getpriority(u64 pid);

/* Set scheduler policy */
int sched_setscheduler(u64 pid, int policy, int priority);

/* Get scheduler statistics */
void scheduler_get_stats(scheduler_stats_t* stats);

/* Calculate virtual runtime (for CFS) */
u64 calc_vruntime(process_t* proc);

#endif /* SCHEDULER_H */
