#ifndef OOM_KILLER_H
#define OOM_KILLER_H

#include "types.h"
#include "process.h"

/* OOM killer modes */
#define OOM_MODE_DISABLED   0
#define OOM_MODE_ENABLED    1
#define OOM_MODE_PANIC      2

/* OOM score calculation */
typedef struct oom_score {
    u64 memory_usage;
    u64 cpu_time;
    u64 children_memory;
    u32 score;
} oom_score_t;

/* Initialize OOM killer */
void oom_killer_init(void);

/* Check for OOM condition */
bool oom_check(void);

/* Kill process to free memory */
int oom_kill_process(void);

/* Calculate OOM score for process */
u32 oom_calculate_score(process_t* proc);

/* Set OOM killer mode */
int oom_set_mode(u32 mode);

/* Get OOM killer mode */
u32 oom_get_mode(void);

#endif /* OOM_KILLER_H */
