#ifndef AI_LOG_H
#define AI_LOG_H

#include "ai_types.h"

void ai_log_init(void);
void ai_log_record(u8 action, u32 pid, u32 value);
u32 ai_log_count(void);
const ai_log_entry_t* ai_log_entry(u32 index);
void ai_log_clear(void);
u64 ai_log_total_decisions(void);

#endif /* AI_LOG_H */
