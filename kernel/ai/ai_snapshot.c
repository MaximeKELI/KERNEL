#include "ai_snapshot.h"
#include "ai_manager.h"
#include "ai_predict.h"
#include "ai_policy.h"
#include "ai_goals.h"
#include "stdio.h"
#include "string.h"

typedef struct {
    bool valid;
    ai_metrics_t metrics;
    ai_predict_state_t pred;
    u32 health;
    u32 policy;
    u32 goal;
} ai_snap_t;

static ai_snap_t baseline;

void ai_snapshot_init(void) {
    memset(&baseline, 0, sizeof(baseline));
}

void ai_snapshot_capture(void) {
    if (!ai_initialized) {
        printk("AI not ready\n");
        return;
    }
    ai_get_metrics(&baseline.metrics);
    ai_predict_get(&baseline.pred);
    baseline.health = ai_health_score();
    baseline.policy = ai_policy_get_mode();
    baseline.goal = ai_goals_get();
    baseline.valid = true;
    printk("[AI] Snapshot saved (health %u)\n", baseline.health);
}

bool ai_snapshot_has_baseline(void) {
    return baseline.valid;
}

void ai_snapshot_compare(void) {
    if (!baseline.valid) {
        printk("No snapshot. Run: ai snapshot\n");
        return;
    }

    ai_metrics_t now_m;
    ai_predict_state_t now_p;
    ai_get_metrics(&now_m);
    ai_predict_get(&now_p);
    u32 now_h = ai_health_score();

    printk("\n[AI] Snapshot delta vs baseline:\n");
    printk("  CPU:    %lld%%\n",
           (long long)now_m.cpu_usage - (long long)baseline.metrics.cpu_usage);
    printk("  MEM:    %lld%%\n",
           (long long)now_m.memory_usage - (long long)baseline.metrics.memory_usage);
    printk("  Health: %lld (%u -> %u)\n",
           (long long)now_h - (long long)baseline.health,
           baseline.health, now_h);
    printk("  NetEMA: %lld\n",
           (long long)now_p.net_ema - (long long)baseline.pred.net_ema);
    printk("\n");
}
