#ifndef AI_LEARN_H
#define AI_LEARN_H

#include "ai_manager.h"
#include "ai_types.h"

void ai_learn_init(void);
void ai_learn_tick(const ai_metrics_t* m);
void ai_learn_on_action(u8 action);
u8 ai_learn_weight(u8 action);
void ai_learn_print_stats(void);
void ai_learn_reset(void);

#endif /* AI_LEARN_H */
