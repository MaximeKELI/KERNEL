#include "ai_network.h"
#include "ai_log.h"
#include "ai_learn.h"
#include "process.h"
#include "scheduler.h"

extern process_t* process_list;

void ai_optimize_network(const ai_policy_params_t* policy, u64 net_rx_rate) {
    if (!policy || net_rx_rate < policy->net_high) {
        return;
    }

    u32 boosted = 0;
    for (process_t* p = process_list; p; p = p->next) {
        if (p->state != PROCESS_BLOCKED || p->ai_class != AI_CLASS_NET) {
            continue;
        }
        if (p->priority > PRIO_MIN) {
            p->priority--;
            p->ai_boost = 4;
            boosted++;
            ai_log_record(AI_ACT_NET_BOOST, (u32)p->pid, (u32)net_rx_rate);
            ai_learn_on_action(AI_ACT_NET_BOOST);
        }
    }
    if (boosted > 0) {
        (void)boosted;
    }
}
