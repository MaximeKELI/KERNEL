#ifndef AI_MONITOR_H
#define AI_MONITOR_H

#include "ai_manager.h"

/* Initialize monitor */
void ai_monitor_init(void);

/* Update metrics */
void ai_monitor_update(void);

/* Get current metrics */
void ai_monitor_get_metrics(ai_metrics_t* metrics);

#endif /* AI_MONITOR_H */
