#include "ai_manager.h"
#include "ai_monitor.h"
#include "ai_optimizer.h"
#include "ai_policy.h"
#include "ai_predict.h"
#include "ai_process.h"
#include "ai_network.h"
#include "ai_log.h"
#include "ai_sysfs.h"
#include "stdio.h"
#include "spinlock.h"

bool ai_initialized = false;
static spinlock_t ai_init_lock = SPINLOCK_INIT;
static u32 ai_tick_phase = 0;

void ai_init(void) {
    spinlock_lock(&ai_init_lock);

    if (ai_initialized) {
        spinlock_unlock(&ai_init_lock);
        return;
    }

    ai_policy_init();
    ai_predict_init();
    ai_log_init();
    ai_process_init();
    ai_monitor_init();
    ai_optimizer_init();
    ai_sysfs_init();

    ai_initialized = true;
    ai_tick_phase = 0;
    spinlock_unlock(&ai_init_lock);

    printk("[AI] Adaptive Intelligence v2 (policy/predict/process/log)\n");
}

void ai_tick(void) {
    if (!ai_initialized) {
        return;
    }

    ai_update_metrics();

    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    ai_predict_feed(&metrics);
    ai_process_update_profiles();

    ai_tick_phase++;

    if (ai_policy_should_run_scheduler(ai_tick_phase)) {
        ai_optimize_scheduler();
    }
    if (ai_policy_should_run_network(ai_tick_phase)) {
        ai_policy_params_t policy;
        ai_policy_get_params(&policy);
        static u64 last_net = 0;
        u64 net = metrics.net_rx_packets + metrics.net_tx_packets;
        u64 rate = (net > last_net) ? (net - last_net) * 100 : 0;
        last_net = net;
        ai_optimize_network(&policy, rate);
    }
    if (ai_policy_should_run_memory(ai_tick_phase)) {
        ai_optimize_memory();
    }
    if ((ai_tick_phase % 50) == 0) {
        ai_detect_anomalies();
    }
}

void ai_get_metrics(ai_metrics_t* metrics) {
    if (!metrics) {
        return;
    }
    ai_monitor_get_metrics(metrics);
}
