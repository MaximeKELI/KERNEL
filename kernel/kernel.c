#include "kernel.h"
#include "kernel_init.h"
#include "kshell.h"
#include "stdio.h"
#include "interrupt.h"
#include "drivers/vga.h"
#include "serial.h"
#include "drivers/timer.h"
#include "workqueue.h"
#include "timer_hr.h"
#include "scheduler.h"
#include "net.h"
#include "log.h"
#include "audit.h"
#include "debug.h"
#include "test.h"

void panic(const char* message) {
    disable_interrupts();
    printk("\n!!! KERNEL PANIC !!!\n");
    printk("%s\n", message);
    printk("System halted.\n");
    halt();
}

void halt(void) {
    while (true) {
        __asm__ __volatile__("cli; hlt");
    }
}

void kernel_main(u64 magic, u64 mb_info) {
    (void)magic;
    (void)mb_info;

    vga_init();
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    serial_init(COM1);

    printk("========================================\n");
    printk("  64-bit OS Kernel v%d.%d.%d\n",
           KERNEL_VERSION_MAJOR,
           KERNEL_VERSION_MINOR,
           KERNEL_VERSION_PATCH);
    printk("========================================\n\n");

    kernel_init_minimal();
    enable_interrupts();

    printk("========================================\n");
    printk("Fast boot complete. Shell ready.\n");
    printk("  init-full  - load all subsystems\n");
    printk("========================================\n\n");

    kshell_init();

#ifdef RUN_TESTS
    printk("Initializing test framework...\n");
    test_init();

    extern void register_memory_tests(void);
    extern void register_scheduler_tests(void);
    extern void register_all_tests(void);
    extern void register_network_tests(void);
    extern void register_ai_tests(void);
    register_memory_tests();
    register_scheduler_tests();
    register_all_tests();
    register_network_tests();
    register_ai_tests();
    printk("Running kernel tests...\n");
    int test_result = test_run_all();
    if (test_result != 0) {
        printk("WARNING: Some tests failed!\n");
    }
#endif

    while (true) {
        if (kernel_extended_ready()) {
            net_poll();
            hrtimer_process();
            extern workqueue_t* system_wq;
            if (system_wq) {
                workqueue_process(system_wq);
            }
            schedule();
        }
        kshell_run_once();
    }
}
