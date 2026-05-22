#include "ai_optimizer.h"
#include "ai_manager.h"
#include "ai_policy.h"
#include "ai_log.h"
#include "process.h"
#include "scheduler.h"
#include "drivers/timer.h"
#include "stdio.h"

extern process_t* process_list;

#define BENCH_ITERATIONS 200

static u64 bench_io_wake_ticks(bool ai_on) {
    ai_set_enabled(ai_on);

    process_state_t st_save[8];
    u64 pri_save[8];
    u32 n = 0;
    for (process_t* p = process_list; p && n < 8; p = p->next) {
        st_save[n] = p->state;
        pri_save[n] = p->priority;
        p->state = PROCESS_BLOCKED;
        p->priority = 10;
        n++;
    }

    u64 start = timer_get_ticks();
    for (u32 i = 0; i < BENCH_ITERATIONS; i++) {
        for (process_t* p = process_list; p; p = p->next) {
            if (p->state == PROCESS_BLOCKED && p->priority > 0) {
                p->priority--;
            }
        }
        if (ai_on) {
            ai_optimize_scheduler();
        }
        schedule();
    }
    u64 end = timer_get_ticks();

    u32 idx = 0;
    for (process_t* p = process_list; p && idx < n; p = p->next, idx++) {
        p->state = st_save[idx];
        p->priority = pri_save[idx];
    }
    ai_set_enabled(true);
    return (end > start) ? (end - start) : 1;
}

void ai_sched_benchmark(void) {
    if (!ai_initialized) {
        printk("[bench-ai] Run init-full first\n");
        return;
    }

    u64 log_before = ai_log_total_decisions();
    u64 without = bench_io_wake_ticks(false);
    u64 with = bench_io_wake_ticks(true);
    u64 log_after = ai_log_total_decisions();
    u64 delta_ms = (without > with) ? (without - with) * 10 : 0;

    printk("\n[bench-ai] I/O scheduler (%u iter, policy %s)\n",
           BENCH_ITERATIONS,
           ai_policy_mode_name(ai_policy_get_mode()));
    printk("  AI off:  %llu ticks (~%llu ms)\n",
           (unsigned long long)without, (unsigned long long)without * 10);
    printk("  AI on:   %llu ticks (~%llu ms)\n",
           (unsigned long long)with, (unsigned long long)with * 10);
    if (with < without) {
        printk("  Gain:    %llu ticks (~%llu ms)\n",
               (unsigned long long)(without - with),
               (unsigned long long)delta_ms);
    } else {
        printk("  Gain:    none this run\n");
    }
    printk("  Decisions during bench: %llu\n",
           (unsigned long long)(log_after - log_before));
    printk("\n");
}
