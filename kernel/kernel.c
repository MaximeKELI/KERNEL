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
#include "drivers/serial.h"
#include "drivers/framebuffer.h"
#include "fs/vfs.h"
#include "fs/inode.h"
#include "fs/dentry.h"
#include "fs/ext2.h"
#include "fs/procfs.h"
#include "fs/sysfs.h"
#include "fs/devtmpfs.h"
#include "signal.h"
#include "ipc.h"
#include "sysv_ipc.h"
#include "device.h"
#include "module.h"
#include "scheduler.h"
#include "cache.h"
#include "log.h"
#include "kthread.h"
#include "workqueue.h"
#include "elf.h"
#include "timer_hr.h"
#include "net.h"
#include "smp.h"
#include "acpi.h"
#include "epoll.h"
#include "slab.h"
#include "rcu.h"
#include "capabilities.h"
#include "namespace.h"
#include "cgroup.h"
#include "kaslr.h"
#include "audit.h"
#include "kvm.h"
#include "ftrace.h"
#include "kprobes.h"
#include "seccomp.h"
#include "block.h"
#include "io_sched.h"
#include "tmpfs.h"
#include "hugepages.h"
#include "watchdog.h"
#include "ai_manager.h"
#include "bpf.h"
#include "livepatch.h"
#include "lsm.h"
#include "numa.h"
#include "hotplug.h"
#include "sched_deadline.h"
#include "kexec.h"
#include "perf.h"
#include "kspp.h"
#include "efi.h"
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
    
    /* Initialize AI Optimization Subsystem (after scheduler and memory) */
    printk("Initializing AI Optimization Subsystem...\n");
    ai_init();
    printk("AI Optimization Subsystem initialized.\n\n");
    
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
    
    /* Initialize proc filesystem */
    printk("Initializing proc filesystem...\n");
    procfs_init();
    printk("Proc filesystem initialized.\n\n");
    
    /* Initialize kernel threads */
    printk("Initializing kernel threads...\n");
    kthread_init();
    printk("Kernel threads initialized.\n\n");
    
    /* Initialize workqueues */
    printk("Initializing workqueues...\n");
    workqueue_init();
    printk("Workqueues initialized.\n\n");
    
    /* Initialize high-resolution timers */
    printk("Initializing high-resolution timers...\n");
    hrtimer_init();
    printk("High-resolution timers initialized.\n\n");
    
    /* Initialize serial port */
    printk("Initializing serial port...\n");
    serial_init(COM1);
    printk("Serial port initialized.\n\n");
    
    /* Initialize networking stack */
    printk("Initializing networking stack...\n");
    net_init();
    printk("Networking stack initialized.\n\n");
    
    /* Initialize SMP */
    printk("Initializing SMP...\n");
    smp_init();
    printk("SMP initialized.\n\n");
    
    /* Initialize ACPI */
    printk("Initializing ACPI...\n");
    acpi_init();
    printk("ACPI initialized.\n\n");
    
    /* Initialize framebuffer */
    printk("Initializing framebuffer...\n");
    framebuffer_init(VESA_MODE_1024x768_8);
    printk("Framebuffer initialized.\n\n");
    
    /* Initialize System V IPC */
    printk("Initializing System V IPC...\n");
    sysv_ipc_init();
    printk("System V IPC initialized.\n\n");
    
    /* Initialize epoll/select */
    printk("Initializing epoll/select...\n");
    epoll_init();
    printk("Epoll/select initialized.\n\n");
    
    /* Initialize SLAB allocator */
    printk("Initializing SLAB allocator...\n");
    slab_init();
    printk("SLAB allocator initialized.\n\n");
    
    /* Initialize RCU */
    printk("Initializing RCU...\n");
    rcu_init();
    printk("RCU initialized.\n\n");
    
    /* Initialize sysfs */
    printk("Initializing sysfs...\n");
    sysfs_init();
    printk("Sysfs initialized.\n\n");
    
    /* Initialize devtmpfs */
    printk("Initializing devtmpfs...\n");
    devtmpfs_init();
    printk("Devtmpfs initialized.\n\n");
    
    /* Initialize capabilities */
    printk("Initializing capabilities...\n");
    capabilities_init();
    printk("Capabilities initialized.\n\n");
    
    /* Initialize namespaces */
    printk("Initializing namespaces...\n");
    namespace_init();
    printk("Namespaces initialized.\n\n");
    
    /* Initialize cgroups */
    printk("Initializing cgroups...\n");
    cgroup_init();
    printk("Cgroups initialized.\n\n");
    
    /* Initialize KASLR */
    printk("Initializing KASLR...\n");
    kaslr_init();
    printk("KASLR initialized.\n\n");
    
    /* Initialize audit system */
    printk("Initializing audit system...\n");
    audit_init();
    printk("Audit system initialized.\n\n");
    
    /* Initialize KVM */
    printk("Initializing KVM...\n");
    kvm_init();
    printk("KVM initialized.\n\n");
    
    /* Initialize ftrace */
    printk("Initializing ftrace...\n");
    ftrace_init();
    printk("Ftrace initialized.\n\n");
    
    /* Initialize kprobes */
    printk("Initializing kprobes...\n");
    kprobes_init();
    printk("Kprobes initialized.\n\n");
    
    /* Initialize seccomp */
    printk("Initializing seccomp...\n");
    seccomp_init();
    printk("Seccomp initialized.\n\n");
    
    /* Initialize block layer */
    printk("Initializing block layer...\n");
    block_init();
    printk("Block layer initialized.\n\n");
    
    /* Initialize I/O schedulers */
    printk("Initializing I/O schedulers...\n");
    io_sched_init();
    printk("I/O schedulers initialized.\n\n");
    
    /* Initialize tmpfs */
    printk("Initializing tmpfs...\n");
    tmpfs_init();
    printk("Tmpfs initialized.\n\n");
    
    /* Initialize huge pages */
    printk("Initializing huge pages...\n");
    hugepages_init();
    printk("Huge pages initialized.\n\n");
    
    /* Initialize watchdog */
    printk("Initializing watchdog...\n");
    watchdog_init();
    printk("Watchdog initialized.\n\n");
    
    /* Initialize BPF */
    printk("Initializing BPF...\n");
    bpf_init();
    printk("BPF initialized.\n\n");
    
    /* Initialize live patching */
    printk("Initializing live patching...\n");
    livepatch_init();
    printk("Live patching initialized.\n\n");
    
    /* Initialize LSM framework */
    printk("Initializing LSM framework...\n");
    lsm_init();
    printk("LSM framework initialized.\n\n");
    
    /* Initialize NUMA */
    printk("Initializing NUMA...\n");
    numa_init();
    printk("NUMA initialized.\n\n");
    
    /* Initialize hotplug */
    printk("Initializing hotplug...\n");
    hotplug_init();
    printk("Hotplug initialized.\n\n");
    
    /* Initialize deadline scheduler */
    printk("Initializing deadline scheduler...\n");
    sched_deadline_init();
    printk("Deadline scheduler initialized.\n\n");
    
    /* Initialize kexec */
    printk("Initializing kexec...\n");
    kexec_init();
    printk("Kexec initialized.\n\n");
    
    /* Initialize perf */
    printk("Initializing perf...\n");
    perf_init();
    printk("Perf initialized.\n\n");
    
    /* Initialize KSPP */
    printk("Initializing KSPP...\n");
    kspp_init();
    printk("KSPP initialized.\n\n");
    
    /* Initialize EFI (if available) */
    printk("Initializing EFI...\n");
    efi_init(NULL); /* Would pass EFI system table from boot */
    printk("EFI initialized.\n\n");
    
    /* Enable interrupts */
    enable_interrupts();
    
    printk("========================================\n");
    printk("Kernel initialization complete!\n");
    printk("System ready.\n");
    printk("========================================\n\n");
    
    log_info("Kernel fully initialized with all subsystems");
    audit_log(AUDIT_CONFIG, "Kernel initialization complete");
    DEBUG_INFO("Debug system initialized (level: %u)", debug_level);
    
    /* Main loop */
    while (true) {
        /* Process high-resolution timers */
        hrtimer_process();
        
        /* Process workqueues */
        extern workqueue_t* system_wq;
        if (system_wq) {
            workqueue_process(system_wq);
        }
        
        asm volatile("hlt");
        schedule();
    }
}
