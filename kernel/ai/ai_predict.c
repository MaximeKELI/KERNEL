#include "ai_predict.h"
#include "spinlock.h"

#define EMA_ALPHA_NUM 3
#define EMA_ALPHA_DEN 10

static ai_predict_state_t state;
static spinlock_t predict_lock = SPINLOCK_INIT;
static u64 last_io_ops = 0;
static u64 last_net_pkt = 0;

static u64 ema_update(u64 prev, u64 sample) {
    if (prev == 0) {
        return sample;
    }
    return (prev * (EMA_ALPHA_DEN - EMA_ALPHA_NUM) + sample * EMA_ALPHA_NUM) /
           EMA_ALPHA_DEN;
}

void ai_predict_init(void) {
    spinlock_lock(&predict_lock);
    memset(&state, 0, sizeof(state));
    spinlock_unlock(&predict_lock);
}

void ai_predict_feed(const ai_metrics_t* m) {
    if (!m) {
        return;
    }

    u64 io_rate = m->io_read_ops + m->io_write_ops;
    u64 net_rate = m->net_rx_packets + m->net_tx_packets;
    if (io_rate > last_io_ops) {
        io_rate = (io_rate - last_io_ops) * 100;
    } else {
        io_rate = 0;
    }
    if (net_rate > last_net_pkt) {
        net_rate = (net_rate - last_net_pkt) * 100;
    } else {
        net_rate = 0;
    }
    last_io_ops = m->io_read_ops + m->io_write_ops;
    last_net_pkt = m->net_rx_packets + m->net_tx_packets;

    spinlock_lock(&predict_lock);
    u64 prev_cpu = state.cpu_ema;
    u64 prev_mem = state.mem_ema;

    state.cpu_ema = ema_update(state.cpu_ema, m->cpu_usage);
    state.mem_ema = ema_update(state.mem_ema, m->memory_usage);
    state.io_ema = ema_update(state.io_ema, io_rate);
    state.net_ema = ema_update(state.net_ema, net_rate);

    state.cpu_trend = (s64)state.cpu_ema - (s64)prev_cpu;
    state.mem_trend = (s64)state.mem_ema - (s64)prev_mem;

    state.cpu_predict = state.cpu_ema + (u64)(state.cpu_trend > 0 ? state.cpu_trend : 0);
    state.mem_predict = state.mem_ema + (u64)(state.mem_trend > 0 ? state.mem_trend : 0);
    if (state.cpu_predict > 100) {
        state.cpu_predict = 100;
    }
    if (state.mem_predict > 100) {
        state.mem_predict = 100;
    }

    spinlock_unlock(&predict_lock);
}

void ai_predict_get(ai_predict_state_t* out) {
    if (!out) {
        return;
    }
    spinlock_lock(&predict_lock);
    *out = state;
    spinlock_unlock(&predict_lock);
}

u64 ai_predict_cpu(void) {
    spinlock_lock(&predict_lock);
    u64 v = state.cpu_predict;
    spinlock_unlock(&predict_lock);
    return v;
}

u64 ai_predict_memory(void) {
    spinlock_lock(&predict_lock);
    u64 v = state.mem_predict;
    spinlock_unlock(&predict_lock);
    return v;
}

s64 ai_predict_cpu_trend(void) {
    spinlock_lock(&predict_lock);
    s64 v = state.cpu_trend;
    spinlock_unlock(&predict_lock);
    return v;
}
