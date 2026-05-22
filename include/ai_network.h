#ifndef AI_NETWORK_H
#define AI_NETWORK_H

#include "ai_policy.h"

void ai_optimize_network(const ai_policy_params_t* policy, u64 net_rx_rate);

#endif /* AI_NETWORK_H */
