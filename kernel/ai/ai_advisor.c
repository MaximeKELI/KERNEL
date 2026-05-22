#include "ai_advisor.h"
#include "ai_manager.h"
#include "ai_predict.h"
#include "ai_policy.h"
#include "ai_goals.h"
#include "ai_controller.h"
#include "ai_process.h"
#include "ai_log.h"
#include "stdio.h"
#include "string.h"

#define ADVICE_MAX 256

static char advice_buf[ADVICE_MAX];

void ai_advisor_init(void) {
    advice_buf[0] = '\0';
}

void ai_advisor_refresh(void) {
    if (!ai_initialized) {
        strncpy(advice_buf, "AI not initialized.", ADVICE_MAX - 1);
        return;
    }

    ai_metrics_t m;
    ai_predict_state_t pred;
    ai_get_metrics(&m);
    ai_predict_get(&pred);

    u32 health = ai_health_score();
    char tmp[ADVICE_MAX];
    int n = 0;

    n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                  "Health %u/100. ", health);

    if (health < 50) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "System stressed; ");
    } else if (health > 85) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "System healthy; ");
    }

    if (pred.net_ema > pred.cpu_ema && pred.net_ema > 300) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "network-heavy -> goal latency or policy auto. ");
    }
    if (m.memory_usage > 80) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "high RAM -> expect mem reclaim. ");
    }
    if (ai_process_count_by_class(AI_CLASS_CPU) > 0 && m.cpu_usage > 70) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "CPU hogs present -> check ai ps. ");
    }
    if (ai_log_count() == 0) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "no decisions yet -> run appliance. ");
    }
    if (ai_goals_get() == AI_GOAL_NONE && !ai_controller_auto_enabled()) {
        n += snprintf(tmp + n, ADVICE_MAX - (size_t)n,
                      "try: ai policy auto. ");
    }

    strncpy(advice_buf, tmp, ADVICE_MAX - 1);
    advice_buf[ADVICE_MAX - 1] = '\0';
}

void ai_advisor_print(void) {
    ai_advisor_refresh();
    printk("\n[AI Advisor]\n%s\n\n", advice_buf);
}
