#ifndef AI_HISTORY_H
#define AI_HISTORY_H

#include "types.h"
#include "ai_types.h"

void ai_history_init(void);
void ai_history_push(u64 cpu, u64 mem, u64 net_rate, u64 io_rate);
void ai_history_print(void);

#endif /* AI_HISTORY_H */
