#include "ai_manager.h"
#include "ai_predict.h"
#include "ai_policy.h"
#include "ai_goals.h"
#include "ai_log.h"
#include "ai_process.h"
#include "ai_types.h"
#include "string.h"

static u32 cached_health = 100;

u32 ai_health_score(void) {
    return cached_health;
}

static void health_recompute(const ai_metrics_t* m, const ai_predict_state_t* pred) {
    if (!m || !pred) {
        cached_health = 0;
        return;
    }

    u32 h = 100;

    if (m->cpu_usage > 90) {
        h -= 25;
    } else if (m->cpu_usage > 75) {
        h -= 12;
    }

    if (m->memory_usage > 90) {
        h -= 30;
    } else if (m->memory_usage > 80) {
        h -= 15;
    }

    if (pred->cpu_trend > 5) {
        h -= 10;
    } else if (pred->cpu_trend < -3) {
        h += 5;
    }

    if (m->context_switches > 400) {
        h -= 8;
    }

    if (m->interrupt_rate > 1500) {
        h -= 7;
    }

    if (ai_log_count() > 20) {
        h -= 5;
    }

    if (h > 100) {
        h = 100;
    }
    cached_health = h;
}

void ai_fill_user_info(ai_user_info_t* out) {
    ai_metrics_t m;
    ai_predict_state_t pred;
    ai_get_metrics(&m);
    ai_predict_get(&pred);
    health_recompute(&m, &pred);

    if (!out) {
        return;
    }

    memset(out, 0, sizeof(*out));
    out->magic = AI_USER_MAGIC;
    out->version = 4;
    out->cpu_usage = (u32)m.cpu_usage;
    out->memory_usage = (u32)m.memory_usage;
    out->cpu_predict = (u32)pred.cpu_predict;
    out->mem_predict = (u32)pred.mem_predict;
    out->health_score = cached_health;
    out->policy_mode = ai_policy_get_mode();
    out->goal_mode = ai_goals_get();
    out->process_count = (u32)m.process_count;
    out->decisions_total = (u32)ai_log_total_decisions();
    out->io_class_count = ai_process_count_by_class(AI_CLASS_IO);
    out->net_class_count = ai_process_count_by_class(AI_CLASS_NET);
}
