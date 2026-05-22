#include "ai_controller.h"
#include "ai_policy.h"
#include "ai_goals.h"
#include "ai_log.h"
#include "drivers/timer.h"

static bool auto_enabled = true;
static u32 last_switch_tick = 0;
static u32 last_mode = AI_POLICY_BALANCED;

void ai_controller_init(void) {
    auto_enabled = true;
    last_switch_tick = 0;
    last_mode = AI_POLICY_BALANCED;
}

bool ai_controller_auto_enabled(void) {
    return auto_enabled;
}

void ai_controller_set_auto(bool on) {
    auto_enabled = on;
}

void ai_controller_tick(const ai_metrics_t* m, const ai_predict_state_t* pred) {
    if (!auto_enabled || !m || !pred || ai_goals_get() != AI_GOAL_NONE) {
        return;
    }

    u32 now = (u32)timer_get_ticks();
    if (now - last_switch_tick < 200) {
        return;
    }

    u32 want = AI_POLICY_BALANCED;

    if (pred->net_ema > 400 || (pred->net_ema > pred->cpu_ema && pred->net_ema > 200)) {
        want = AI_POLICY_LATENCY;
    } else if (pred->cpu_ema > 75 && pred->cpu_trend > 0) {
        want = AI_POLICY_BALANCED;
    } else if (pred->cpu_ema > 85) {
        want = AI_POLICY_THROUGHPUT;
    } else if (pred->io_ema > 600) {
        want = AI_POLICY_LATENCY;
    } else if (pred->cpu_ema < 12 && pred->net_ema < 80 && pred->io_ema < 100) {
        want = AI_POLICY_POWERSAVE;
    } else if (m->context_switches > 200) {
        want = AI_POLICY_LATENCY;
    }

    if (want != last_mode) {
        ai_policy_set_mode(want);
        ai_log_record(AI_ACT_POLICY_AUTO, 0, want);
        last_mode = want;
        last_switch_tick = now;
    }
}
