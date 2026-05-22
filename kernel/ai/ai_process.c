#include "ai_process.h"
#include "ai_log.h"
#include "ai_predict.h"
#include "scheduler.h"
#include "stdio.h"

extern process_t* process_list;

#define WAIT_THRESHOLD 5
#define BOOST_MAX 8

void ai_process_init(void) {
}

static u8 classify_process(process_t* p, u64 net_rx_rate) {
    if (!p || p->state == PROCESS_DEAD || p->state == PROCESS_ZOMBIE) {
        return AI_CLASS_UNKNOWN;
    }

    u32 total = (u32)p->ai_wait_ticks + (u32)p->ai_run_ticks;
    if (total < WAIT_THRESHOLD) {
        return AI_CLASS_IDLE;
    }

    u32 wait_pct = (p->ai_wait_ticks * 100) / (total ? total : 1);

    if (wait_pct > 70 && net_rx_rate > 100) {
        return AI_CLASS_NET;
    }
    if (wait_pct > 60) {
        return AI_CLASS_IO;
    }
    if (wait_pct < 20 && p->runtime > 50) {
        return AI_CLASS_CPU;
    }
    if (wait_pct < 40) {
        return AI_CLASS_MIXED;
    }
    return AI_CLASS_IDLE;
}

void ai_process_update_profiles(void) {
    for (process_t* p = process_list; p; p = p->next) {
        if (p->state == PROCESS_DEAD || p->state == PROCESS_ZOMBIE) {
            continue;
        }

        if (p->state == PROCESS_BLOCKED) {
            if (p->ai_wait_ticks < 65535) {
                p->ai_wait_ticks++;
            }
        } else if (p->state == PROCESS_RUNNING || p->state == PROCESS_READY) {
            if (p->ai_run_ticks < 65535) {
                p->ai_run_ticks++;
            }
        }

        if (p->ai_boost > 0) {
            p->ai_boost--;
        }
    }
}

void ai_process_apply_scheduler(const ai_policy_params_t* policy,
                                u64 net_rx_rate, u64 io_rate) {
    if (!policy) {
        return;
    }

    u64 cpu_pred = ai_predict_cpu();
    u64 cpu_now = cpu_pred;
    i64 trend = ai_predict_cpu_trend();

    for (process_t* p = process_list; p; p = p->next) {
        p->ai_class = classify_process(p, net_rx_rate);
    }

    if (cpu_now > policy->cpu_high || (trend > 2 && cpu_now > 50)) {
        u32 n = 0;
        for (process_t* p = process_list; p; p = p->next) {
            if (p->time_slice > 10) {
                p->time_slice = p->time_slice * 3 / 4;
                n++;
            }
        }
        if (n > 0) {
            ai_log_record(AI_ACT_SLICE_REDUCE, 0, n);
        }
    }

    if (io_rate > policy->io_high || net_rx_rate > policy->net_high) {
        u32 boosted = 0;
        for (process_t* p = process_list; p; p = p->next) {
            if (p->state != PROCESS_BLOCKED) {
                continue;
            }
            if (p->ai_class != AI_CLASS_IO && p->ai_class != AI_CLASS_NET) {
                continue;
            }
            if (p->priority > PRIO_MIN && p->ai_boost < BOOST_MAX) {
                p->priority--;
                p->ai_boost++;
                boosted++;
                ai_log_record(AI_ACT_PRIO_BOOST, (u32)p->pid, (u32)p->priority);
            }
        }
        (void)boosted;
    }

    if (cpu_now < 20) {
        u32 n = 0;
        for (process_t* p = process_list; p; p = p->next) {
            if (p->time_slice < 200) {
                p->time_slice = p->time_slice * 5 / 4;
                n++;
            }
        }
        if (n > 0) {
            ai_log_record(AI_ACT_SLICE_INCREASE, 0, n);
        }
    }
}

process_t* ai_process_top_cpu_hog(void) {
    process_t* best = NULL;
    u64 max_rt = 0;
    for (process_t* p = process_list; p; p = p->next) {
        if (p->state == PROCESS_RUNNING && p->runtime > max_rt) {
            max_rt = p->runtime;
            best = p;
        }
    }
    return best;
}

u32 ai_process_count_by_class(u8 class_id) {
    u32 n = 0;
    for (process_t* p = process_list; p; p = p->next) {
        if (p->ai_class == class_id) {
            n++;
        }
    }
    return n;
}
