#include "ai_goals.h"
#include "ai_policy.h"
#include "ai_controller.h"
#include "ai_log.h"
#include "ai_types.h"
#include "string.h"

static u32 current_goal = AI_GOAL_NONE;

static const char* goal_names[] = {
    "none", "latency", "throughput", "power", "stability",
};

void ai_goals_init(void) {
    current_goal = AI_GOAL_NONE;
}

u32 ai_goals_get(void) {
    return current_goal;
}

const char* ai_goals_name(u32 goal) {
    if (goal > AI_GOAL_STABILITY) {
        return "unknown";
    }
    return goal_names[goal];
}

void ai_goals_set(u32 goal) {
    if (goal <= AI_GOAL_STABILITY) {
        current_goal = goal;
    }
}

void ai_goals_apply(void) {
    if (current_goal == AI_GOAL_NONE) {
        return;
    }

    ai_controller_set_auto(false);
    u32 pol = AI_POLICY_BALANCED;

    switch (current_goal) {
    case AI_GOAL_LATENCY:
        pol = AI_POLICY_LATENCY;
        break;
    case AI_GOAL_THROUGHPUT:
        pol = AI_POLICY_THROUGHPUT;
        break;
    case AI_GOAL_POWER:
        pol = AI_POLICY_POWERSAVE;
        break;
    case AI_GOAL_STABILITY:
        pol = AI_POLICY_BALANCED;
        ai_controller_set_auto(false);
        break;
    default:
        return;
    }

    ai_policy_set_mode(pol);
    ai_log_record(AI_ACT_GOAL_APPLY, current_goal, pol);
}
