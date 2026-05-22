#ifndef AI_GOALS_H
#define AI_GOALS_H

#include "types.h"

void ai_goals_init(void);
u32 ai_goals_get(void);
const char* ai_goals_name(u32 goal);
void ai_goals_set(u32 goal);
void ai_goals_apply(void);

#endif /* AI_GOALS_H */
