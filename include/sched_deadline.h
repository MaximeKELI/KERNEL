#ifndef SCHED_DEADLINE_H
#define SCHED_DEADLINE_H

#include "types.h"
#include "process.h"

/* Deadline scheduling */
#define SCHED_DEADLINE 6

/* Deadline parameters */
typedef struct {
    u64 runtime;    /* Execution time per period */
    u64 deadline;   /* Relative deadline */
    u64 period;     /* Period */
} sched_dl_param_t;

/* Initialize deadline scheduler */
void sched_deadline_init(void);

/* Set deadline parameters */
int sched_setattr_deadline(u64 pid, sched_dl_param_t* params);

/* Get deadline parameters */
int sched_getattr_deadline(u64 pid, sched_dl_param_t* params);

/* Schedule deadline task */
void sched_deadline_schedule(void);

#endif /* SCHED_DEADLINE_H */
