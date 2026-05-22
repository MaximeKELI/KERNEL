#ifndef AI_CONTROLLER_H
#define AI_CONTROLLER_H

#include "ai_manager.h"
#include "ai_predict.h"

void ai_controller_init(void);
void ai_controller_tick(const ai_metrics_t* m, const ai_predict_state_t* pred);
bool ai_controller_auto_enabled(void);
void ai_controller_set_auto(bool on);

#endif /* AI_CONTROLLER_H */
