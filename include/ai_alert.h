#ifndef AI_ALERT_H
#define AI_ALERT_H

#include "ai_manager.h"
#include "ai_predict.h"

void ai_alert_init(void);
void ai_alert_check(const ai_metrics_t* m, const ai_predict_state_t* pred);
void ai_alert_print_recent(void);
void ai_alert_reset(void);

#endif /* AI_ALERT_H */
