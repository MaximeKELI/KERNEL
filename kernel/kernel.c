#include "kernel.h"
#include "kernel_init.h"
#include "boot_profiler.h"
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
#include "multiboot2_fb.h"
#include "init.h"

u64 kernel_mb_info = 0;

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
    kernel_mb_info = mb_info;
    (void)magic;

    vga_init();
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    serial_init(COM1);

    printk("Kernel v%d.%d.%d [fast boot]\n",
           KERNEL_VERSION_MAJOR,
           KERNEL_VERSION_MINOR,
           KERNEL_VERSION_PATCH);

    kernel_init_minimal();
    enable_interrupts();
    boot_profiler_mark("shell");

    printk("Shell ready. init-full | score | appliance | exec nettest\n");

#ifdef RUN_TESTS
    /* No init in the test build: the in-kernel shell drives the console. */
    kshell_init();
#endif

#ifdef RUN_TESTS
    printk("Initializing test framework...\n");
    test_init();

    extern void register_memory_tests(void);
    extern void register_scheduler_tests(void);
    extern void register_all_tests(void);
    extern void register_network_tests(void);
    extern void register_ai_tests(void);
    extern void register_roadmap_tests(void);
    extern void register_phases_abcd_tests(void);
    extern void register_usermode_tests(void);
    extern void register_signal_tests(void);
    extern void register_fs_tests(void);
    extern void register_abi_tests(void);
    register_memory_tests();
    register_scheduler_tests();
    register_all_tests();
    register_network_tests();
    register_ai_tests();
    register_roadmap_tests();
    register_phases_abcd_tests();
    register_usermode_tests();
    register_signal_tests();
    register_fs_tests();
    register_abi_tests();
    printk("Running kernel tests...\n");
    int test_result = test_run_all();
    if (test_result != 0) {
        printk("WARNING: Some tests failed!\n");
    }
#endif

#ifndef RUN_TESTS
    /* Boot the real user journey: PID 1 launches /sh in ring 3. */
    init_start();
#endif

    while (true) {
        if (kernel_net_ready()) {
            net_poll();
            hrtimer_process();
            extern workqueue_t* system_wq;
            if (system_wq) {
                workqueue_process(system_wq);
            }
        }
        if (kernel_extended_ready()) {
            extern void writeback_tick(void);
            extern void vdso_update(void);
            writeback_tick();
            vdso_update();
        }
        if (g_init_active) {
            /* init owns the console; just let its threads run. */
            schedule();
            __asm__ __volatile__("hlt");
        } else {
            schedule();
            kshell_run_once();
        }
    }
}




















