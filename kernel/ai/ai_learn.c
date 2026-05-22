#include "ai_learn.h"
#include "ai_predict.h"
#include "ai_log.h"
#include "stdio.h"
#include "string.h"

static u8 weights[AI_LEARN_ACTIONS];
static u8 last_action = AI_ACT_NONE;
static u64 prev_cpu = 0;
static u64 prev_mem = 0;
static u32 reward_count = 0;
static u32 penalize_count = 0;

void ai_learn_init(void) {
    memset(weights, 128, sizeof(weights));
    last_action = AI_ACT_NONE;
    prev_cpu = 0;
    prev_mem = 0;
    reward_count = 0;
    penalize_count = 0;
}

void ai_learn_on_action(u8 action) {
    if (action < AI_LEARN_ACTIONS) {
        last_action = action;
    }
}

static void adjust(u8 action, bool reward) {
    if (action >= AI_LEARN_ACTIONS) {
        return;
    }
    if (reward) {
        if (weights[action] < 250) {
            weights[action] += 8;
        }
        reward_count++;
        ai_log_record(AI_ACT_LEARN_REWARD, action, weights[action]);
    } else {
        if (weights[action] > 10) {
            weights[action] -= 5;
        }
        penalize_count++;
    }
}

void ai_learn_tick(const ai_metrics_t* m) {
    if (!m || last_action == AI_ACT_NONE) {
        prev_cpu = m ? m->cpu_usage : 0;
        prev_mem = m ? m->memory_usage : 0;
        return;
    }

    u8 act = last_action;
    last_action = AI_ACT_NONE;

    switch (act) {
    case AI_ACT_SLICE_REDUCE:
        adjust(act, m->cpu_usage < prev_cpu || m->context_switches < 300);
        break;
    case AI_ACT_SLICE_INCREASE:
        adjust(act, m->cpu_usage < 50);
        break;
    case AI_ACT_PRIO_BOOST:
    case AI_ACT_NET_BOOST:
        adjust(act, m->context_switches < prev_cpu + 50);
        break;
    case AI_ACT_PRIO_DEMOTE:
        adjust(act, m->cpu_usage < prev_cpu);
        break;
    case AI_ACT_MEM_RECLAIM:
        adjust(act, m->memory_usage < prev_mem);
        break;
    default:
        break;
    }

    prev_cpu = m->cpu_usage;
    prev_mem = m->memory_usage;
}

u8 ai_learn_weight(u8 action) {
    if (action >= AI_LEARN_ACTIONS) {
        return 128;
    }
    return weights[action];
}

void ai_learn_print_stats(void) {
    printk("Learn weights (128=neutral):\n");
    printk("  slice-:%u slice+:%u boost:%u demote:%u\n",
           weights[AI_ACT_SLICE_REDUCE], weights[AI_ACT_SLICE_INCREASE],
           weights[AI_ACT_PRIO_BOOST], weights[AI_ACT_PRIO_DEMOTE]);
    printk("  mem:%u net:%u | rewards:%u penalties:%u\n",
           weights[AI_ACT_MEM_RECLAIM], weights[AI_ACT_NET_BOOST],
           reward_count, penalize_count);
}

void ai_learn_reset(void) {
    ai_learn_init();
}
