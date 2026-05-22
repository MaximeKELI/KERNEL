#ifndef SCHED_STATS_H
#define SCHED_STATS_H

#include "types.h"

typedef struct {
    u64 context_switches;
    u64 schedule_calls;
    u64 nr_running;
} sched_stats_t;

void sched_stats_init(void);
void sched_stats_record_switch(void);
void sched_stats_record_schedule(void);
void sched_stats_get(sched_stats_t* out);

#endif /* SCHED_STATS_H */
