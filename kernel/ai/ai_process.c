#include "ai_process.h"
#include "ai_log.h"
#include "ai_predict.h"
#include "ai_learn.h"
#include "scheduler.h"
#include "stdio.h"

extern process_t* process_list;

#define WAIT_THRESHOLD 5
#define BOOST_MAX 12

static const char* class_names[] = {
    "?", "idle", "cpu", "io", "net", "mix",
};

const char* ai_process_class_name(u8 class_id) {
    if (class_id > AI_CLASS_MIXED) {
        return "?";
    }
    return class_names[class_id];
}

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
    u32 run_pct = 100 - wait_pct;

    if (wait_pct > 70 && net_rx_rate > 100) {
        p->ai_score = (u8)(wait_pct > 100 ? 100 : wait_pct);
        return AI_CLASS_NET;
    }
    if (wait_pct > 60) {
        p->ai_score = (u8)wait_pct;
        return AI_CLASS_IO;
    }
    if (run_pct > 80 && p->runtime > 30) {
        p->ai_score = (u8)run_pct;
        return AI_CLASS_CPU;
    }
    if (wait_pct < 40) {
        p->ai_score = (u8)((wait_pct + run_pct) / 2);
        return AI_CLASS_MIXED;
    }
    p->ai_score = (u8)(100 - wait_pct);
    return AI_CLASS_IDLE;
}

void ai_process_update_profiles(void) {
    ai_predict_state_t pred;
    ai_predict_get(&pred);

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

        p->ai_class = classify_process(p, pred.net_ema);

        if (p->ai_boost > 0) {
            p->ai_boost--;
        }
    }
}

static u32 learn_scale(u8 action, u32 base) {
    u8 w = ai_learn_weight(action);
    return (base * (u32)w) / 128;
}

void ai_process_apply_scheduler(const ai_policy_params_t* policy,
                                u64 net_rx_rate, u64 io_rate) {
    if (!policy) {
        return;
    }

    u64 cpu_pred = ai_predict_cpu();
    i64 trend = ai_predict_cpu_trend();
    u32 slice_reduce_thresh = learn_scale(AI_ACT_SLICE_REDUCE, (u32)policy->cpu_high);

    if (cpu_pred > slice_reduce_thresh || (trend > 2 && cpu_pred > 50)) {
        u32 n = 0;
        for (process_t* p = process_list; p; p = p->next) {
            if (p->ai_class == AI_CLASS_CPU && p->time_slice > 10) {
                p->time_slice = p->time_slice * 2 / 3;
                n++;
            } else if (p->time_slice > 10 && cpu_pred > policy->cpu_high) {
                p->time_slice = p->time_slice * 3 / 4;
                n++;
            }
        }
        if (n > 0) {
            ai_log_record(AI_ACT_SLICE_REDUCE, 0, n);
            ai_learn_on_action(AI_ACT_SLICE_REDUCE);
        }
    }

    u32 io_thresh = learn_scale(AI_ACT_PRIO_BOOST, (u32)policy->io_high);
    u32 net_thresh = learn_scale(AI_ACT_NET_BOOST, (u32)policy->net_high);

    if (io_rate > io_thresh || net_rx_rate > net_thresh) {
        for (process_t* p = process_list; p; p = p->next) {
            if (p->state != PROCESS_BLOCKED) {
                continue;
            }
            if (p->ai_class != AI_CLASS_IO && p->ai_class != AI_CLASS_NET) {
                continue;
            }
            if (p->priority > PRIO_MIN && p->ai_boost < BOOST_MAX) {
                u8 boost_steps = (p->ai_score / 25) + 1;
                if (boost_steps > 3) {
                    boost_steps = 3;
                }
                p->priority -= boost_steps;
                if (p->priority < PRIO_MIN) {
                    p->priority = PRIO_MIN;
                }
                p->ai_boost = BOOST_MAX / 2;
                ai_log_record(AI_ACT_PRIO_BOOST, (u32)p->pid, (u32)p->priority);
                ai_learn_on_action(AI_ACT_PRIO_BOOST);
            }
        }
    }

    if (cpu_pred < 20) {
        u32 n = 0;
        for (process_t* p = process_list; p; p = p->next) {
            if (p->ai_class == AI_CLASS_IDLE || p->ai_class == AI_CLASS_IO) {
                if (p->time_slice < 200) {
                    p->time_slice = p->time_slice * 5 / 4;
                    n++;
                }
            }
        }
        if (n > 0) {
            ai_log_record(AI_ACT_SLICE_INCREASE, 0, n);
            ai_learn_on_action(AI_ACT_SLICE_INCREASE);
        }
    }

    u32 tuned = 0;
    for (process_t* p = process_list; p; p = p->next) {
        if (p->ai_class == AI_CLASS_NET && p->time_slice < 150) {
            p->time_slice += 10;
            tuned++;
        }
    }
    if (tuned > 0) {
        ai_log_record(AI_ACT_VRUNTIME_TUNED, 0, tuned);
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

void ai_process_print_table(void) {
    printk("  PID  CLASS  SCORE  BOOST  WAIT   RUN    PRIO  SLICE\n");
    for (process_t* p = process_list; p; p = p->next) {
        if (p->state == PROCESS_DEAD || p->state == PROCESS_ZOMBIE) {
            continue;
        }
        printk("  %3llu %-5s %3u   %3u   %4u  %4u  %4llu %4llu\n",
               (unsigned long long)p->pid,
               ai_process_class_name(p->ai_class),
               p->ai_score, p->ai_boost,
               p->ai_wait_ticks, p->ai_run_ticks,
               (unsigned long long)p->priority,
               (unsigned long long)p->time_slice);
    }
}
