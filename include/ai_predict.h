#ifndef AI_PREDICT_H
#define AI_PREDICT_H

#include "ai_manager.h"
#include "ai_types.h"

void ai_predict_init(void);
void ai_predict_feed(const ai_metrics_t* m);
void ai_predict_get(ai_predict_state_t* out);
u64 ai_predict_cpu(void);
u64 ai_predict_memory(void);
s64 ai_predict_cpu_trend(void);

#endif /* AI_PREDICT_H */
