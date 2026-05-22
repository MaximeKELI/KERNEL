#include "ai_optimizer.h"
#include "ai_monitor.h"
#include "ai_policy.h"
#include "ai_predict.h"
#include "ai_process.h"
#include "ai_network.h"
#include "ai_log.h"
#include "process.h"
#include "cache.h"
#include "drivers/timer.h"
#include "debug.h"
#include "validate.h"

static bool optimizer_initialized = false;
static u64 last_memory_cleanup = 0;
static u64 cpu_high_threshold = 80;
static u64 memory_pressure_threshold = 85;
static u64 anomaly_detection_ticks = 100;
static bool ai_enabled = true;

extern process_t* process_list;

static u64 metric_io_rate(const ai_metrics_t* m) {
    static u64 last_r, last_w;
    u64 r = m->io_read_ops;
    u64 w = m->io_write_ops;
    u64 rate = 0;
    if (r + w > last_r + last_w) {
        rate = (r + w - last_r - last_w) * 100;
    }
    last_r = r;
    last_w = w;
    return rate;
}

static u64 metric_net_rate(const ai_metrics_t* m) {
    static u64 last_rx, last_tx;
    u64 rx = m->net_rx_packets;
    u64 tx = m->net_tx_packets;
    u64 rate = 0;
    if (rx + tx > last_rx + last_tx) {
        rate = (rx + tx - last_rx - last_tx) * 100;
    }
    last_rx = rx;
    last_tx = tx;
    return rate;
}

void ai_optimizer_init(void) {
    optimizer_initialized = true;
}

void ai_optimize_scheduler(void) {
    if (!optimizer_initialized || !ai_enabled) {
        return;
    }

    ai_policy_params_t policy;
    ai_policy_get_params(&policy);
    cpu_high_threshold = policy.cpu_high;
    memory_pressure_threshold = policy.mem_high;

    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);

    u64 io_rate = metric_io_rate(&metrics);
    u64 net_rate = metric_net_rate(&metrics);

    ai_process_apply_scheduler(&policy, net_rate, io_rate);

    if (ai_predict_cpu() > policy.cpu_high && ai_predict_cpu_trend() > 3) {
        ai_log_record(AI_ACT_ANOMALY, 0, (u32)ai_predict_cpu());
    }
}

void ai_optimize_memory(void) {
    if (!optimizer_initialized || !ai_enabled) {
        return;
    }

    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    u64 mem_pred = ai_predict_memory();

    if (metrics.memory_usage > memory_pressure_threshold ||
        mem_pred > memory_pressure_threshold) {
        u64 now = timer_get_ticks();
        if (now - last_memory_cleanup > 1000) {
            cache_sync_all();
            ai_log_record(AI_ACT_MEM_RECLAIM, 0, (u32)metrics.memory_usage);
            last_memory_cleanup = now;
        }
    }
}

void ai_detect_anomalies(void) {
    if (!optimizer_initialized || !ai_enabled) {
        return;
    }

    ai_metrics_t metrics;
    ai_monitor_get_metrics(&metrics);
    static u64 high_cpu_ticks = 0;

    if (metrics.cpu_usage > cpu_high_threshold) {
        high_cpu_ticks++;
        if (high_cpu_ticks > anomaly_detection_ticks) {
            process_t* hog = ai_process_top_cpu_hog();
            if (hog && hog->priority < PRIO_MAX) {
                hog->priority++;
                ai_log_record(AI_ACT_PRIO_DEMOTE, (u32)hog->pid, (u32)hog->priority);
            }
            high_cpu_ticks = 0;
        }
    } else {
        high_cpu_ticks = 0;
    }

    if (metrics.interrupt_rate > 1000) {
        ai_log_record(AI_ACT_ANOMALY, 0, (u32)metrics.interrupt_rate);
    }
}

u64 ai_get_cpu_threshold(void) {
    return cpu_high_threshold;
}

void ai_set_cpu_threshold(u64 threshold) {
    if (threshold > 0 && threshold <= 100) {
        cpu_high_threshold = threshold;
    }
}

u64 ai_get_memory_threshold(void) {
    return memory_pressure_threshold;
}

void ai_set_memory_threshold(u64 threshold) {
    if (threshold > 0 && threshold <= 100) {
        memory_pressure_threshold = threshold;
    }
}

bool ai_is_enabled(void) {
    return ai_enabled;
}

void ai_set_enabled(bool enabled) {
    ai_enabled = enabled;
}
