#ifndef SCHED_TUNE_H
#define SCHED_TUNE_H

#include "types.h"

void sched_tune_init(void);
int sched_tune_set_boost(u64 pid, i32 boost);
int sched_tune_clear_boost(u64 pid);
i32 sched_tune_get_boost(u64 pid);

#endif /* SCHED_TUNE_H */
