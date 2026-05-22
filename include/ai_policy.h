#ifndef AI_POLICY_H
#define AI_POLICY_H

#include "types.h"
#include "ai_types.h"

typedef struct {
    u64 cpu_high;
    u64 mem_high;
    u64 cs_high;
    u64 io_high;
    u64 net_high;
    u8 scheduler_divisor;
    u8 memory_divisor;
} ai_policy_params_t;

void ai_policy_init(void);
u32 ai_policy_get_mode(void);
const char* ai_policy_mode_name(u32 mode);
void ai_policy_set_mode(u32 mode);
void ai_policy_get_params(ai_policy_params_t* out);
bool ai_policy_should_run_scheduler(u32 tick_phase);
bool ai_policy_should_run_memory(u32 tick_phase);
bool ai_policy_should_run_network(u32 tick_phase);

#endif /* AI_POLICY_H */
