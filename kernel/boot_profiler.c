#include "boot_profiler.h"
#include "asm.h"
#include "drivers/timer.h"
#include "stdio.h"
#include "string.h"

#define BOOT_PHASE_MAX 12
#define BOOT_PHASE_NAME 24

typedef struct {
    char name[BOOT_PHASE_NAME];
    u64 tsc;
    u64 tick;
} boot_phase_t;

static boot_phase_t phases[BOOT_PHASE_MAX];
static u32 phase_count = 0;
static u64 tsc_origin = 0;
static u64 tick_origin = 0;

void boot_profiler_reset(void) {
    phase_count = 0;
    tsc_origin = rdtsc_asm();
    tick_origin = timer_get_ticks();
}

void boot_profiler_mark(const char* phase) {
    if (!phase || phase_count >= BOOT_PHASE_MAX) {
        return;
    }
    boot_phase_t* p = &phases[phase_count++];
    strncpy(p->name, phase, BOOT_PHASE_NAME - 1);
    p->name[BOOT_PHASE_NAME - 1] = '\0';
    p->tsc = rdtsc_asm();
    p->tick = timer_get_ticks();
}

static u64 profiler_delta_ms(u64 tick_a, u64 tick_b) {
    if (tick_b <= tick_a) {
        return 0;
    }
    return (tick_b - tick_a) * 10;
}

u64 boot_profiler_ms(const char* phase) {
    for (u32 i = 0; i < phase_count; i++) {
        if (strcmp(phases[i].name, phase) == 0) {
            if (i == 0) {
                return profiler_delta_ms(tick_origin, phases[i].tick);
            }
            return profiler_delta_ms(phases[i - 1].tick, phases[i].tick);
        }
    }
    return 0;
}

u64 boot_profiler_total_ms(void) {
    if (phase_count == 0) {
        return 0;
    }
    return profiler_delta_ms(tick_origin, phases[phase_count - 1].tick);
}

void boot_profiler_report(void) {
    printk("\n[Boot] Timeline (timer @100Hz):\n");
    u64 prev_tick = tick_origin;
    for (u32 i = 0; i < phase_count; i++) {
        u64 ms = profiler_delta_ms(prev_tick, phases[i].tick);
        printk("  %-20s %4llu ms\n",
               phases[i].name, (unsigned long long)ms);
        prev_tick = phases[i].tick;
    }
    printk("  %-20s %4llu ms (total)\n",
           "TOTAL", (unsigned long long)boot_profiler_total_ms());
}
