#ifndef AI_PROCESS_H
#define AI_PROCESS_H

#include "types.h"
#include "process.h"

void ai_process_init(void);
void ai_process_update_profiles(void);
void ai_process_apply_scheduler(const ai_policy_params_t* policy,
                                u64 net_rx_rate, u64 io_rate);
process_t* ai_process_top_cpu_hog(void);
u32 ai_process_count_by_class(u8 class_id);

#endif /* AI_PROCESS_H */
