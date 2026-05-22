#include "ai_shell.h"
#include "ai_manager.h"
#include "ai_policy.h"
#include "ai_predict.h"
#include "ai_log.h"
#include "ai_process.h"
#include "ai_optimizer.h"
#include "ai_bench.h"
#include "ai_controller.h"
#include "ai_learn.h"
#include "ai_history.h"
#include "kernel_init.h"
#include "stdio.h"
#include "string.h"

static const char* act_name(u8 a) {
    switch (a) {
    case AI_ACT_SLICE_REDUCE: return "slice-";
    case AI_ACT_SLICE_INCREASE: return "slice+";
    case AI_ACT_PRIO_BOOST: return "boost";
    case AI_ACT_PRIO_DEMOTE: return "demote";
    case AI_ACT_MEM_RECLAIM: return "mem";
    case AI_ACT_NET_BOOST: return "net";
    case AI_ACT_ANOMALY: return "anomaly";
    case AI_ACT_POLICY_AUTO: return "auto-pol";
    case AI_ACT_LEARN_REWARD: return "learn+";
    case AI_ACT_VRUNTIME_TUNED: return "vruntime";
    default: return "?";
    }
}

static void ai_cmd_status(void) {
    if (!ai_initialized) {
        printk("AI: not initialized (init-full)\n");
        return;
    }

    ai_metrics_t m;
    ai_get_metrics(&m);
    ai_predict_state_t pred;
    ai_predict_get(&pred);

    printk("\n=== AI Subsystem ===\n");
    printk("Enabled:    %s\n", ai_is_enabled() ? "yes" : "no");
    printk("Policy:     %s (auto: %s)\n",
           ai_policy_mode_name(ai_policy_get_mode()),
           ai_controller_auto_enabled() ? "on" : "off");
    printk("Decisions:  %llu total, %u in log\n",
           (unsigned long long)ai_log_total_decisions(),
           ai_log_count());
    printk("CPU:        %llu%% (pred %llu%%, trend %+lld)\n",
           (unsigned long long)m.cpu_usage,
           (unsigned long long)pred.cpu_predict,
           (long long)pred.cpu_trend);
    printk("Memory:     %llu%% (pred %llu%%)\n",
           (unsigned long long)m.memory_usage,
           (unsigned long long)pred.mem_predict);
    printk("Processes:  %llu (cpu:%u io:%u net:%u idle:%u)\n",
           (unsigned long long)m.process_count,
           ai_process_count_by_class(AI_CLASS_CPU),
           ai_process_count_by_class(AI_CLASS_IO),
           ai_process_count_by_class(AI_CLASS_NET),
           ai_process_count_by_class(AI_CLASS_IDLE));
    printk("I/O:        read %llu write %llu ops\n",
           (unsigned long long)m.io_read_ops,
           (unsigned long long)m.io_write_ops);
    printk("Network:    rx %llu tx %llu pkts\n",
           (unsigned long long)m.net_rx_packets,
           (unsigned long long)m.net_tx_packets);
    printk("Scheduler:  %llu ctx/s, %llu irq/s\n",
           (unsigned long long)m.context_switches,
           (unsigned long long)m.interrupt_rate);
    printk("Thresholds: CPU %llu%% MEM %llu%%\n",
           (unsigned long long)ai_get_cpu_threshold(),
           (unsigned long long)ai_get_memory_threshold());
    printk("Predict:    io_ema %llu net_ema %llu\n",
           (unsigned long long)pred.io_ema,
           (unsigned long long)pred.net_ema);
    ai_learn_print_stats();
    printk("\n");
}

static void ai_cmd_log(void) {
    u32 n = ai_log_count();
    if (n == 0) {
        printk("AI log empty\n");
        return;
    }
    printk("AI decisions (last %u):\n", n);
    for (u32 i = 0; i < n; i++) {
        const ai_log_entry_t* e = ai_log_entry(i);
        if (!e) {
            break;
        }
        printk("  tick %llu [%s] pid=%u val=%u pol=%u\n",
               (unsigned long long)e->tick,
               act_name(e->action),
               e->pid, e->value, e->policy);
    }
}

static void ai_cmd_policy(const char* name) {
    if (!name || !name[0]) {
        printk("Policy: %s\n", ai_policy_mode_name(ai_policy_get_mode()));
        printk("Modes: balanced | latency | throughput | powersave | auto | manual\n");
        return;
    }
    if (strcmp(name, "auto") == 0) {
        ai_controller_set_auto(true);
        printk("AI auto-policy enabled\n");
        return;
    }
    if (strcmp(name, "manual") == 0) {
        ai_controller_set_auto(false);
        printk("AI auto-policy disabled (manual)\n");
        return;
    }
    for (u32 m = 0; m < AI_POLICY_MAX; m++) {
        if (strcmp(name, ai_policy_mode_name(m)) == 0) {
            ai_policy_set_mode(m);
            ai_controller_set_auto(false);
            printk("AI policy -> %s (manual)\n", ai_policy_mode_name(m));
            return;
        }
    }
    printk("Unknown policy: %s\n", name);
}

static void ai_cmd_tune(const char* args) {
    if (!args || !strncmp(args, "cpu ", 4)) {
        u64 v = 0;
        const char* p = args ? args + 4 : "";
        while (*p >= '0' && *p <= '9') {
            v = v * 10 + (u64)(*p - '0');
            p++;
        }
        if (v > 0 && v <= 100) {
            ai_set_cpu_threshold(v);
            printk("CPU threshold -> %llu%%\n", (unsigned long long)v);
        }
        return;
    }
    if (!strncmp(args, "mem ", 4)) {
        u64 v = 0;
        const char* p = args + 4;
        while (*p >= '0' && *p <= '9') {
            v = v * 10 + (u64)(*p - '0');
            p++;
        }
        if (v > 0 && v <= 100) {
            ai_set_memory_threshold(v);
            printk("MEM threshold -> %llu%%\n", (unsigned long long)v);
        }
        return;
    }
    printk("Usage: ai tune cpu N | ai tune mem N\n");
}

void ai_shell_command(const char* args) {
    if (!args || !args[0]) {
        ai_cmd_status();
        return;
    }
    if (strcmp(args, "status") == 0 || strcmp(args, "metrics") == 0) {
        ai_cmd_status();
    } else if (strcmp(args, "log") == 0) {
        ai_cmd_log();
    } else if (strcmp(args, "bench") == 0) {
        if (!kernel_extended_ready()) {
            printk("Need init-full\n");
        } else {
            ai_sched_benchmark();
        }
    } else if (strncmp(args, "policy ", 7) == 0) {
        ai_cmd_policy(args + 7);
    } else if (strcmp(args, "policy") == 0) {
        ai_cmd_policy(NULL);
    } else if (strncmp(args, "on", 2) == 0) {
        ai_set_enabled(true);
        printk("AI enabled\n");
    } else if (strncmp(args, "off", 3) == 0) {
        ai_set_enabled(false);
        printk("AI disabled\n");
    } else if (strncmp(args, "clear", 5) == 0) {
        ai_log_clear();
        printk("AI log cleared\n");
    } else if (strncmp(args, "tune ", 5) == 0) {
        ai_cmd_tune(args + 5);
    } else if (strcmp(args, "history") == 0) {
        ai_history_print();
    } else if (strcmp(args, "learn") == 0) {
        ai_learn_print_stats();
    } else if (strcmp(args, "ps") == 0) {
        ai_process_print_table();
    } else if (strcmp(args, "reset-learn") == 0) {
        ai_learn_reset();
        printk("AI learn weights reset\n");
    } else if (strcmp(args, "help") == 0) {
        printk("AI commands:\n");
        printk("  ai              - status\n");
        printk("  ai metrics      - full metrics\n");
        printk("  ai policy NAME  - mode or auto|manual\n");
        printk("  ai history      - metric sparklines\n");
        printk("  ai ps           - processes + AI class/score\n");
        printk("  ai learn        - reinforcement weights\n");
        printk("  ai log          - decision history\n");
        printk("  ai bench        - scheduler benchmark\n");
        printk("  ai on|off       - enable/disable\n");
        printk("  ai tune cpu N   - CPU threshold\n");
        printk("  ai reset-learn  - reset learn weights\n");
        printk("  ai clear        - clear log\n");
    } else {
        printk("Unknown: ai %s (try: ai help)\n", args);
    }
}
