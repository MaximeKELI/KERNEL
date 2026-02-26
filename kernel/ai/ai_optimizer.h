#ifndef AI_OPTIMIZER_H
#define AI_OPTIMIZER_H

#include "types.h"

/* Initialize optimizer */
void ai_optimizer_init(void);

/* Optimize scheduler */
void ai_optimize_scheduler(void);

/* Optimize memory */
void ai_optimize_memory(void);

/* Detect anomalies */
void ai_detect_anomalies(void);

#endif /* AI_OPTIMIZER_H */
