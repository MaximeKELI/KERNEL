#ifndef AI_MANAGER_H
#define AI_MANAGER_H

#include "types.h"

/* AI metrics structure */
typedef struct {
    u64 cpu_usage;
    u64 memory_usage;
    u64 process_count;
    u64 context_switches;
    u64 interrupt_rate;
} ai_metrics_t;

/* Initialize AI subsystem */
void ai_init(void);

/* AI tick - called from timer interrupt */
void ai_tick(void);

/* Get current metrics */
void ai_get_metrics(ai_metrics_t* metrics);

/* Check if AI subsystem is initialized */
extern bool ai_initialized;

#endif /* AI_MANAGER_H */
