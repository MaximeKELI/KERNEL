#include "ai_daemon.h"
#include "ai_advisor.h"
#include "ai_alert.h"
#include "ai_goals.h"
#include "ai_manager.h"
#include "ai_predict.h"
#include "ai_health.h"

#define DAEMON_INTERVAL 100

void ai_daemon_init(void) {
}

void ai_daemon_tick(u32 phase) {
    if (!ai_initialized || (phase % DAEMON_INTERVAL) != 0) {
        return;
    }

    ai_metrics_t m;
    ai_predict_state_t pred;
    ai_get_metrics(&m);
    ai_predict_get(&pred);

    ai_fill_user_info(NULL);

    ai_goals_apply();
    ai_alert_check(&m, &pred);
    ai_advisor_refresh();
}
