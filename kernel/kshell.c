#include "kshell.h"
#include "kernel_init.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include "process.h"
#include "drivers/keyboard.h"
#include "blk_mq.h"
#include "landlock.h"
#include "net.h"
#include "net_addr.h"
#include "net_stats.h"
#include "icmp.h"
#include "net_ports.h"
#include "net_socket.h"
#include "serial.h"
#include "hw_ports.h"
#include "dhcp.h"
#include "dns.h"
#include "kernel_score.h"
#include "exec.h"
#include "appliance.h"
#include "ai_bench.h"
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
    printk("  ifconfig   - show network interface\n");
    printk("  ping IP    - ICMP echo (needs init-full)\n");
    printk("  netstats   - RX/TX counters\n");
    printk("  ports      - well-known TCP/UDP ports\n");
    printk("  serial     - COM1-COM4 status\n");
    printk("  ioports    - hardware I/O port map (x86)\n");
    printk("  ioport N   - lookup port 0xN (hex ok)\n");
    printk("  dhcp       - DHCP acquire\n");
    printk("  dns NAME   - DNS A lookup\n");
    printk("  score      - kernel scorecard vs Linux\n");
    printk("  appliance  - DHCP + ping QEMU gateway\n");
    printk("  bench-ai   - AI vs non-AI I/O sched latency\n");
    printk("  exec PATH  - run user ELF (e.g. nettest)\n");
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

static void kshell_cmd_ifconfig(void) {
    netif_t* iface = net_default_if();
    if (!iface) {
        printk("No network interface (run init-full)\n");
        return;
    }
    char ip[16], mask[16], gw[16];
    ip_addr_format(&iface->ip, ip, sizeof(ip));
    ip_addr_format(&iface->netmask, mask, sizeof(mask));
    ip_addr_format(&iface->gateway, gw, sizeof(gw));
    printk("%s: %s up=%s\n", iface->name, ip, iface->up ? "yes" : "no");
    printk("  netmask %s gateway %s\n", mask, gw);
    printk("  mac %02x:%02x:%02x:%02x:%02x:%02x\n",
           iface->mac[0], iface->mac[1], iface->mac[2],
           iface->mac[3], iface->mac[4], iface->mac[5]);
}

static void kshell_cmd_netstats(void) {
    net_stats_t stats;
    net_stats_get(&stats);
    printk("net: rx=%llu tx=%llu rx_err=%llu tx_err=%llu\n",
           (unsigned long long)stats.rx_bytes,
           (unsigned long long)stats.tx_bytes,
           (unsigned long long)stats.rx_errors,
           (unsigned long long)stats.tx_errors);
}

static void kshell_cmd_ports(void) {
    printk("Port ranges: priv 1-%u user %u-%u dyn %u-%u\n",
           NET_PORT_PRIVILEGED_MAX, NET_PORT_USER_MIN, NET_PORT_USER_MAX,
           NET_PORT_DYNAMIC_MIN, NET_PORT_DYNAMIC_MAX);
    printk("Services:\n");
    u16 list[] = {PORT_SSH, PORT_NAMESERVER, PORT_HTTP, PORT_HTTPS,
                  PORT_NTP, PORT_MQTT, PORT_MYSQL, PORT_POSTGRES, PORT_REDIS};
    for (u32 i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
        const char* svc = net_port_service_name(list[i]);
        if (svc) {
            printk("  %5u %-12s (%s)\n", list[i], svc,
                   net_port_classify(list[i]) == NET_PORT_TYPE_PRIVILEGED ?
                   "priv" : "user");
        }
    }
    printk("Socket types: %s %s %s %s %s %s\n",
           net_sock_type_name(SOCK_STREAM), net_sock_type_name(SOCK_DGRAM),
           net_sock_type_name(SOCK_RAW), net_sock_type_name(SOCK_RDM),
           net_sock_type_name(SOCK_SEQPACKET), net_sock_type_name(SOCK_PACKET));
}

static void kshell_cmd_ioports(void) {
    printk("Hardware I/O ports (%u regions):\n", hw_port_registry_count());
    hw_port_list_category(HW_PORT_CAT_UNKNOWN);
}

static u16 kshell_parse_hex_port(const char* s) {
    u32 val = 0;
    if (!s || !*s) {
        return 0;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (*s) {
        char c = *s++;
        u8 digit = 0;
        if (c >= '0' && c <= '9') {
            digit = (u8)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (u8)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = (u8)(c - 'A' + 10);
        } else {
            break;
        }
        val = (val << 4) | digit;
        if (val > 0xFFFF) {
            return 0;
        }
    }
    return (u16)val;
}

static void kshell_cmd_ioport_lookup(const char* arg) {
    while (*arg == ' ') {
        arg++;
    }
    u16 port = kshell_parse_hex_port(arg);
    if (port == 0 && (arg[0] != '0' || (arg[1] != 'x' && arg[1] != 'X'))) {
        printk("Usage: ioport 0xNN\n");
        return;
    }
    const char* name = hw_port_lookup_name(port);
    printk("0x%04X: %s [%s]\n", port,
           name ? name : "(unknown)",
           hw_port_category_name(hw_port_category(port)));
}

static void kshell_cmd_serial(void) {
    for (u32 i = 0; i < SERIAL_PORT_COUNT; i++) {
        u16 base = serial_port_by_name(
            (i == 0) ? "COM1" : (i == 1) ? "COM2" : (i == 2) ? "COM3" : "COM4");
        const char* name = serial_port_name(base);
        printk("  %s 0x%04x  %s%s\n", name ? name : "?", base,
               serial_port_is_init(base) ? "up" : "down",
               (serial_console_enabled && serial_console_port == base) ?
               " (console)" : "");
    }
}

static void kshell_cmd_ping(const char* target) {
    if (!kernel_extended_ready()) {
        printk("Network not loaded. Run: init-full\n");
        return;
    }
    while (*target == ' ') {
        target++;
    }
    if (!*target) {
        printk("Usage: ping A.B.C.D\n");
        return;
    }
    ip_addr_t dst;
    if (!ip_addr_parse(target, &dst)) {
        printk("Invalid address: %s\n", target);
        return;
    }
    icmp_ping_reset_stats();
    if (icmp_ping(dst, 1, 1) < 0) {
        printk("ping send failed\n");
        return;
    }
    for (u32 i = 0; i < 256; i++) {
        net_poll();
    }
    printk("ping: %u reply(s)\n", icmp_ping_replies_received());
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
            kernel_score_print();
        } else {
            kernel_init_extended();
            kernel_score_print();
        }
    } else if (strcmp(cmd, "ifconfig") == 0) {
        kshell_cmd_ifconfig();
    } else if (strcmp(cmd, "netstats") == 0) {
        kshell_cmd_netstats();
    } else if (strncmp(cmd, "ping ", 5) == 0) {
        kshell_cmd_ping(cmd + 5);
    } else if (strcmp(cmd, "ports") == 0) {
        kshell_cmd_ports();
    } else if (strcmp(cmd, "serial") == 0) {
        kshell_cmd_serial();
    } else if (strcmp(cmd, "ioports") == 0) {
        kshell_cmd_ioports();
    } else if (strncmp(cmd, "ioport ", 7) == 0) {
        kshell_cmd_ioport_lookup(cmd + 7);
    } else if (strcmp(cmd, "dhcp") == 0) {
        netif_t* iface = net_default_if();
        if (iface) {
            net_dhcp_acquire(iface);
        }
    } else if (strcmp(cmd, "score") == 0) {
        kernel_score_print();
    } else if (strcmp(cmd, "appliance") == 0) {
        appliance_network_boot();
    } else if (strcmp(cmd, "bench-ai") == 0) {
        ai_sched_benchmark();
    } else if (strncmp(cmd, "exec ", 5) == 0) {
        if (!kernel_extended_ready()) {
            printk("Need init-full for userland.\n");
        } else if (exec_run_path(cmd + 5) < 0) {
            printk("exec failed: %s (try: exec nettest)\n", cmd + 5);
        }
    } else if (strcmp(cmd, "nettest") == 0) {
        if (!kernel_extended_ready()) {
            printk("Need init-full.\n");
        } else if (exec_run_path("/nettest") < 0) {
            printk("nettest not found (rebuild: make iso)\n");
        }
    } else if (strncmp(cmd, "dns ", 4) == 0) {
        ip_addr_t ip;
        if (dns_resolve_a(cmd + 4, &ip) == 0) {
            char b[16];
            ip_addr_format(&ip, b, sizeof(b));
            printk("DNS: %s\n", b);
        } else {
            printk("DNS failed\n");
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
