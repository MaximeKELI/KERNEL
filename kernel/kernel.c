#include "kernel.h"
#include "stdio.h"
#include "memory.h"
#include "interrupt.h"
#include "process.h"
#include "syscall.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "drivers/ata.h"
#include "fs/vfs.h"
#include "io.h"
#include "debug.h"
#include "spinlock.h"

/* Kernel stack is defined in boot.asm */

void panic(const char* message) {
    disable_interrupts();
    printk("\n!!! KERNEL PANIC !!!\n");
    printk("%s\n", message);
    printk("System halted.\n");
    halt();
}

void halt(void) {
    while (true) {
        asm volatile("cli; hlt");
    }
}

void kernel_main(u64 magic, u64 mb_info) {
    (void)magic;
    (void)mb_info;
    
    /* Initialize VGA first */
    vga_init();
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    printk("========================================\n");
    printk("  64-bit OS Kernel v%d.%d.%d\n",
           KERNEL_VERSION_MAJOR,
           KERNEL_VERSION_MINOR,
           KERNEL_VERSION_PATCH);
    printk("========================================\n\n");
    
    /* Initialize memory management */
    printk("Initializing memory management...\n");
    pmm_init(512 * 1024 * 1024, 0x100000, 0x100000);  /* 512MB, reserve first 1MB */
    vmm_init();
    heap_init();
    printk("Memory management initialized.\n\n");
    
    /* Initialize interrupts */
    printk("Initializing interrupts...\n");
    interrupt_init();
    printk("Interrupts initialized.\n\n");
    
    /* Initialize drivers */
    printk("Initializing drivers...\n");
    keyboard_init();
    timer_init(100);  /* 100 Hz */
    ata_init();
    printk("Drivers initialized.\n\n");
    
    /* Initialize VFS */
    printk("Initializing VFS...\n");
    vfs_init();
    printk("VFS initialized.\n\n");
    
    /* Initialize process manager */
    printk("Initializing process manager...\n");
    process_init();
    printk("Process manager initialized.\n\n");
    
    /* Initialize syscalls */
    printk("Initializing syscalls...\n");
    syscall_init();
    printk("Syscalls initialized.\n\n");
    
    /* Enable interrupts */
    enable_interrupts();
    
    printk("========================================\n");
    printk("Kernel initialization complete!\n");
    printk("System ready.\n");
    printk("========================================\n\n");
    
    DEBUG_INFO("Debug system initialized (level: %u)", debug_level);
    
    /* Main loop */
    while (true) {
        asm volatile("hlt");
        schedule();
    }
}
