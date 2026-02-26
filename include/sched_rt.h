#ifndef SCHED_RT_H
#define SCHED_RT_H

#include "types.h"
#include "process.h"

/* Real-time scheduling policies */
#define SCHED_FIFO  1
#define SCHED_RR    2
#define SCHED_NORMAL 0
#define SCHED_BATCH  3
#define SCHED_IDLE   5
#define SCHED_DEADLINE 6

/* Real-time priority range */
#define RT_PRIO_MIN  1
#define RT_PRIO_MAX  99

/* Real-time scheduling parameters */
typedef struct sched_param {
    i32 sched_priority;
} sched_param_t;

/* Set real-time scheduling policy */
int sched_setscheduler(u64 pid, i32 policy, const sched_param_t* param);

/* Get real-time scheduling policy */
int sched_getscheduler(u64 pid);

/* Set real-time priority */
int sched_setparam(u64 pid, const sched_param_t* param);

/* Get real-time priority */
int sched_getparam(u64 pid, sched_param_t* param);

/* Yield CPU (for SCHED_RR) */
int sched_yield(void);

/* Get priority range */
int sched_get_priority_min(i32 policy);
int sched_get_priority_max(i32 policy);

#endif /* SCHED_RT_H */
