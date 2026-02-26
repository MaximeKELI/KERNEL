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
#include "drivers/pci.h"
#include "fs/vfs.h"
#include "fs/inode.h"
#include "fs/dentry.h"
#include "fs/ext2.h"
#include "signal.h"
#include "ipc.h"
#include "device.h"
#include "module.h"
#include "scheduler.h"
#include "cache.h"
#include "log.h"
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
    
    /* Initialize signals */
    printk("Initializing signal system...\n");
    signal_init();
    printk("Signal system initialized.\n\n");
    
    /* Initialize IPC */
    printk("Initializing IPC...\n");
    ipc_init();
    printk("IPC initialized.\n\n");
    
    /* Initialize inode system */
    printk("Initializing inode system...\n");
    inode_init();
    printk("Inode system initialized.\n\n");
    
    /* Initialize dentry system */
    printk("Initializing dentry system...\n");
    dentry_init();
    printk("Dentry system initialized.\n\n");
    
    /* Initialize device manager */
    printk("Initializing device manager...\n");
    device_init();
    printk("Device manager initialized.\n\n");
    
    /* Initialize module system */
    printk("Initializing module system...\n");
    module_init();
    printk("Module system initialized.\n\n");
    
    /* Initialize advanced scheduler */
    printk("Initializing CFS scheduler...\n");
    scheduler_init();
    printk("CFS scheduler initialized.\n\n");
    
    /* Initialize cache system */
    printk("Initializing cache system...\n");
    cache_init();
    printk("Cache system initialized.\n\n");
    
    /* Initialize logging */
    printk("Initializing logging system...\n");
    log_init();
    printk("Logging system initialized.\n\n");
    
    /* Initialize PCI */
    printk("Initializing PCI...\n");
    pci_init();
    printk("PCI initialized.\n\n");
    
    /* Initialize EXT2 filesystem */
    printk("Initializing EXT2 filesystem...\n");
    ext2_init();
    printk("EXT2 filesystem initialized.\n\n");
    
    /* Enable interrupts */
    enable_interrupts();
    
    printk("========================================\n");
    printk("Kernel initialization complete!\n");
    printk("System ready.\n");
    printk("========================================\n\n");
    
    log_info("Kernel fully initialized with all subsystems");
    DEBUG_INFO("Debug system initialized (level: %u)", debug_level);
    
    /* Main loop */
    while (true) {
        asm volatile("hlt");
        schedule();
    }
}
