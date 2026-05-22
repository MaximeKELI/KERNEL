#include "ai_policy.h"
#include "string.h"
#include "stdio.h"

static u32 current_mode = AI_POLICY_BALANCED;

static const ai_policy_params_t policy_table[AI_POLICY_MAX] = {
    [AI_POLICY_BALANCED] = {
        .cpu_high = 80, .mem_high = 85, .cs_high = 50,
        .io_high = 800, .net_high = 500,
        .scheduler_divisor = 2, .memory_divisor = 10,
    },
    [AI_POLICY_LATENCY] = {
        .cpu_high = 65, .mem_high = 75, .cs_high = 30,
        .io_high = 400, .net_high = 200,
        .scheduler_divisor = 1, .memory_divisor = 5,
    },
    [AI_POLICY_THROUGHPUT] = {
        .cpu_high = 90, .mem_high = 90, .cs_high = 80,
        .io_high = 2000, .net_high = 2000,
        .scheduler_divisor = 4, .memory_divisor = 20,
    },
    [AI_POLICY_POWERSAVE] = {
        .cpu_high = 92, .mem_high = 92, .cs_high = 120,
        .io_high = 1500, .net_high = 1500,
        .scheduler_divisor = 8, .memory_divisor = 40,
    },
};

static const char* mode_names[] = {
    "balanced", "latency", "throughput", "powersave",
};

void ai_policy_init(void) {
    current_mode = AI_POLICY_BALANCED;
}

u32 ai_policy_get_mode(void) {
    return current_mode;
}

const char* ai_policy_mode_name(u32 mode) {
    if (mode >= AI_POLICY_MAX) {
        return "unknown";
    }
    return mode_names[mode];
}

void ai_policy_set_mode(u32 mode) {
    if (mode < AI_POLICY_MAX) {
        current_mode = mode;
    }
}

void ai_policy_get_params(ai_policy_params_t* out) {
    if (!out) {
        return;
    }
    *out = policy_table[current_mode];
}

bool ai_policy_should_run_scheduler(u32 tick_phase) {
    return (tick_phase % policy_table[current_mode].scheduler_divisor) == 0;
}

bool ai_policy_should_run_memory(u32 tick_phase) {
    return (tick_phase % policy_table[current_mode].memory_divisor) == 0;
}

bool ai_policy_should_run_network(u32 tick_phase) {
    return (tick_phase % policy_table[current_mode].scheduler_divisor) == 0;
}
