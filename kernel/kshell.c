#include "kshell.h"
#include "kernel_init.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include "process.h"
#include "drivers/keyboard.h"
#include "blk_mq.h"
#include "landlock.h"
#include "types.h"

#define KSHELL_LINE_MAX 128

static char line_buf[KSHELL_LINE_MAX];
static u32 line_len = 0;
static bool kshell_ready = false;

extern process_t* process_list;

static void kshell_prompt(void) {
    printk("\nkshell> ");
}

static void kshell_cmd_help(void) {
    printk("Commands:\n");
    printk("  help       - this message\n");
    printk("  mem        - free/total memory\n");
    printk("  ps         - process list\n");
    printk("  blk        - blk-mq stats\n");
    printk("  landlock   - landlock rules\n");
    printk("  init-full  - load remaining subsystems\n");
}

static void kshell_cmd_mem(void) {
    size_t free_p = pmm_get_free_pages();
    size_t total_p = pmm_get_total_pages();
    printk("Memory: %zu MiB free / %zu MiB total\n",
           (free_p * PAGE_SIZE) / (1024 * 1024),
           (total_p * PAGE_SIZE) / (1024 * 1024));
}

static const char* kshell_state_name(process_state_t state) {
    switch (state) {
    case PROCESS_RUNNING: return "RUN";
    case PROCESS_READY:   return "RDY";
    case PROCESS_BLOCKED: return "BLK";
    case PROCESS_ZOMBIE:  return "ZMB";
    case PROCESS_DEAD:    return "DED";
    default:              return "?";
    }
}

static void kshell_cmd_ps(void) {
    process_t* p = process_list;
    if (!p) {
        printk("(no processes)\n");
        return;
    }
    printk("  PID  STATE  PRIO\n");
    while (p) {
        printk("  %3llu  %-5s  %llu\n",
               (unsigned long long)p->pid,
               kshell_state_name(p->state),
               (unsigned long long)p->priority);
        p = p->next;
    }
}

static void kshell_cmd_blk(void) {
    u64 sub = 0, done = 0;
    blk_mq_get_stats(0, &sub, &done);
    printk("blk-mq: queues=%u submitted=%llu completed=%llu\n",
           blk_mq_queue_count(),
           (unsigned long long)sub,
           (unsigned long long)done);
}

static void kshell_cmd_landlock(void) {
    printk("landlock: %u active rules\n", landlock_rule_count());
}

static void kshell_execute(const char* cmd) {
    if (!cmd[0]) {
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        kshell_cmd_help();
    } else if (strcmp(cmd, "mem") == 0) {
        kshell_cmd_mem();
    } else if (strcmp(cmd, "ps") == 0) {
        kshell_cmd_ps();
    } else if (strcmp(cmd, "blk") == 0) {
        kshell_cmd_blk();
    } else if (strcmp(cmd, "landlock") == 0) {
        kshell_cmd_landlock();
    } else if (strcmp(cmd, "init-full") == 0) {
        if (kernel_extended_ready()) {
            printk("Extended init already done.\n");
        } else {
            kernel_init_extended();
        }
    } else {
        printk("Unknown command: %s (try help)\n", cmd);
    }
}

void kshell_init(void) {
    line_len = 0;
    line_buf[0] = '\0';
    kshell_ready = true;
    printk("\n[kshell] Type 'help' for commands.\n");
    kshell_prompt();
}

void kshell_run_once(void) {
    if (!kshell_ready) {
        return;
    }

    char c = keyboard_read_char();

    if (c == '\b' || c == 127) {
        if (line_len > 0) {
            line_len--;
            line_buf[line_len] = '\0';
            printk("\b \b");
        }
        return;
    }

    if (c == '\r' || c == '\n') {
        line_buf[line_len] = '\0';
        kshell_execute(line_buf);
        line_len = 0;
        line_buf[0] = '\0';
        kshell_prompt();
        return;
    }

    if (line_len + 1 < KSHELL_LINE_MAX && c >= 32 && c < 127) {
        line_buf[line_len++] = c;
        line_buf[line_len] = '\0';
        printk("%c", c);
    }
}
