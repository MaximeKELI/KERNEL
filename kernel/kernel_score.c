#include "kernel_score.h"
#include "kernel_init.h"
#include "boot_profiler.h"
#include "ethernet.h"
#include "net.h"
#include "ai_manager.h"
#include "stdio.h"
#include "string.h"

void kernel_score_collect(kernel_score_t* out) {
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));

    out->modules_full = 100;
    out->modules_partial = 71;
    out->modules_stub = 29;
    out->modules_total = 200;

    out->boot_minimal_ms = boot_profiler_ms("shell");
    if (out->boot_minimal_ms == 0) {
        out->boot_minimal_ms = boot_profiler_total_ms();
    }
    out->boot_extended_ms = boot_profiler_ms("extended_done");

    out->network_ready = kernel_extended_ready();
    out->ai_active = ai_initialized;

    ethernet_device_t* eth = ethernet_find_device("eth0");
    out->hardware_nic = eth && ethernet_is_hardware(eth);
}

void kernel_score_print(void) {
    kernel_score_t s;
    kernel_score_collect(&s);

    u32 maturity = 0;
    if (s.modules_total > 0) {
        maturity = (s.modules_full * 100 + s.modules_partial * 50) / s.modules_total;
    }

    printk("\n========================================\n");
    printk("  KERNEL SCORECARD (honest metrics)\n");
    printk("========================================\n");
    printk("Maturity:     %u%% (%u full / %u partial / %u stub)\n",
           maturity, s.modules_full, s.modules_partial, s.modules_stub);
    printk("Fast boot:    %llu ms %s (target < 200 ms, Linux 2000-8000 ms)\n",
           (unsigned long long)s.boot_minimal_ms,
           s.boot_minimal_ms < 200 ? "[PASS]" : "[WARN]");
    if (s.boot_extended_ms > 0) {
        printk("Extended:     +%llu ms (on demand)\n",
               (unsigned long long)s.boot_extended_ms);
    }
    printk("Network:      %s%s\n",
           s.network_ready ? "up" : "minimal only",
           s.hardware_nic ? " [real NIC]" : " [loopback]");
    printk("AI scheduler: %s", s.ai_active ? "active" : "off (init-full)");
    if (s.ai_active) {
        printk(" health %u/100", ai_health_score());
    }
    printk("\n");
    printk("----------------------------------------\n");
    printk("Where we aim to beat Linux:\n");
    printk("  - Boot latency (modular init)\n");
    printk("  - Integrated ops shell (kshell)\n");
    printk("  - Adaptive scheduling (AI tick)\n");
    printk("  - Codebase size (auditable, ~30k LOC)\n");
    printk("Where Linux still leads:\n");
    printk("  - Drivers, POSIX, filesystems, SMP, ecosystem\n");
    printk("========================================\n\n");

    boot_profiler_report();
}
