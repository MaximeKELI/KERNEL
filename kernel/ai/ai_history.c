#include "ai_history.h"
#include "stdio.h"
#include "string.h"

typedef struct {
    u64 cpu;
    u64 mem;
    u64 net;
    u64 io;
} ai_hist_sample_t;

static ai_hist_sample_t ring[AI_HISTORY_LEN];
static u32 head = 0;
static u32 count = 0;

void ai_history_init(void) {
    head = 0;
    count = 0;
    memset(ring, 0, sizeof(ring));
}

void ai_history_push(u64 cpu, u64 mem, u64 net_rate, u64 io_rate) {
    ai_hist_sample_t* s = &ring[head];
    s->cpu = cpu > 100 ? 100 : cpu;
    s->mem = mem > 100 ? 100 : mem;
    s->net = net_rate > 999 ? 999 : net_rate;
    s->io = io_rate > 999 ? 999 : io_rate;
    head = (head + 1) % AI_HISTORY_LEN;
    if (count < AI_HISTORY_LEN) {
        count++;
    }
}

static void sparkline(const char* label, u64 max_val,
                      u64 (*getter)(const ai_hist_sample_t*)) {
    printk("%-6s ", label);
    u32 start = (head + AI_HISTORY_LEN - count) % AI_HISTORY_LEN;
    for (u32 i = 0; i < count; i++) {
        u32 idx = (start + i) % AI_HISTORY_LEN;
        const ai_hist_sample_t* sample = &ring[idx];
        u64 v = getter(sample);
        (void)sample;
        u32 bars = max_val ? (v * 8 / max_val) : 0;
        if (bars > 8) {
            bars = 8;
        }
        char c = (bars == 0) ? '.' : (char)('0' + bars);
        printk("%c", c);
    }
    printk("\n");
}

static u64 get_cpu(const ai_hist_sample_t* s) {
    return s->cpu;
}
static u64 get_mem(const ai_hist_sample_t* s) {
    return s->mem;
}
static u64 get_net(const ai_hist_sample_t* s) {
    return s->net / 100;
}
static u64 get_io(const ai_hist_sample_t* s) {
    return s->io / 100;
}

void ai_history_print(void) {
    if (count == 0) {
        printk("AI history empty\n");
        return;
    }
    printk("AI history (%u samples, 0-8 scale):\n", count);
    sparkline("cpu%", 100, get_cpu);
    sparkline("mem%", 100, get_mem);
    sparkline("net", 9, get_net);
    sparkline("io", 9, get_io);
}
