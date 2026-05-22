#include "ai_alert.h"
#include "ai_log.h"
#include "stdio.h"
#include "string.h"

#define ALERT_MSG_MAX 96

static char last_alert[ALERT_MSG_MAX];
static u8 fired_mask = 0;

enum {
    ALERT_CPU = 1,
    ALERT_MEM = 2,
    ALERT_NET = 4,
    ALERT_PRED = 8,
};

void ai_alert_init(void) {
    last_alert[0] = '\0';
    fired_mask = 0;
}

static void fire(u8 bit, const char* msg) {
    if (fired_mask & bit) {
        return;
    }
    fired_mask |= bit;
    strncpy(last_alert, msg, ALERT_MSG_MAX - 1);
    last_alert[ALERT_MSG_MAX - 1] = '\0';
    printk("[AI ALERT] %s\n", msg);
    ai_log_record(AI_ACT_ALERT, bit, 0);
}

void ai_alert_check(const ai_metrics_t* m, const ai_predict_state_t* pred) {
    if (!m || !pred) {
        return;
    }

    if (m->cpu_usage > 92) {
        fire(ALERT_CPU, "CPU critical >92%");
    } else if (m->cpu_usage < 70) {
        fired_mask &= ~ALERT_CPU;
    }

    if (m->memory_usage > 90) {
        fire(ALERT_MEM, "Memory critical >90%");
    } else if (m->memory_usage < 75) {
        fired_mask &= ~ALERT_MEM;
    }

    if (pred->net_ema > 800) {
        fire(ALERT_NET, "Network storm (high RX/TX EMA)");
    } else if (pred->net_ema < 400) {
        fired_mask &= ~ALERT_NET;
    }

    if (pred->cpu_predict > 95 && pred->cpu_trend > 3) {
        fire(ALERT_PRED, "CPU spike predicted");
    } else if (pred->cpu_predict < 80) {
        fired_mask &= ~ALERT_PRED;
    }
}

void ai_alert_print_recent(void) {
    if (last_alert[0]) {
        printk("Last alert: %s\n", last_alert);
    } else {
        printk("No active alerts\n");
    }
}

void ai_alert_reset(void) {
    ai_alert_init();
}
