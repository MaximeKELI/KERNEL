#include "kernel_init.h"
#include "boot_profiler.h"
#include "stdio.h"
#include "memory.h"
#include "interrupt.h"
#include "process.h"
#include "syscall.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "hw_ports.h"
#include "drivers/ata.h"
#include "pci.h"
#include "fs/vfs.h"
#include "inode.h"
#include "dentry.h"
#include "ext2.h"
#include "procfs.h"
#include "sysfs.h"
#include "devtmpfs.h"
#include "signal.h"
#include "ipc.h"
#include "sysv_ipc.h"
#include "device.h"
#include "module.h"
#include "scheduler.h"
#include "sched_stats.h"
#include "sched_tune.h"
#include "fs_checksum.h"
#include "cache.h"
#include "log.h"
#include "kthread.h"
#include "workqueue.h"
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
#include "blk_mq.h"
#include "io_sched.h"
#include "landlock.h"
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
#include "io_uring.h"
#include "fuse.h"
#include "iommu.h"
#include "energy.h"
#include "devicetree.h"
#include "kasan.h"
#include "overlayfs.h"
#include "security_hardening.h"
#include "tpm.h"
#include "secure_random.h"
#include "memory_encryption.h"
#include "mac.h"
#include "secure_boot.h"
#include "rop_protection.h"
#include "rdma.h"
#include "dpdk.h"
#include "xdp.h"
#include "nvme.h"
#include "container.h"
#include "checkpoint.h"
#include "framebuffer.h"
#include "kernel.h"
#include "media.h"
#include "memory_pressure.h"
#include "kswapd.h"
#include "memory_defrag.h"
#include "memory_overcommit.h"
#include "thp.h"
#include "memcg.h"
#include "load_balance.h"
#include "sched_domain.h"
#include "debug.h"
#include "appliance_config.h"
#include "virtio_gpu.h"
#include "ahci.h"
#include "fb_console.h"
#include "pc_speaker.h"
#include "ebpf.h"

static bool g_extended_ready = false;

bool kernel_extended_ready(void) {
    return g_extended_ready;
}

void kernel_init_minimal(void) {
    printk("[init] fast boot path\n");

    printk("Memory (core)...\n");
    pmm_init(512 * 1024 * 1024, 0x100000, 0x100000);
    vmm_init();
    heap_init();

    interrupt_init();
    hw_ports_init();
    keyboard_init();
    timer_init(100);
    boot_profiler_reset();

    process_init();
    syscall_init();
    scheduler_init();

    boot_profiler_mark("minimal_done");
}

void kernel_init_extended(void) {
    if (g_extended_ready) {
        return;
    }
    printk("[init] loading extended subsystems...\n");
    boot_profiler_mark("extended_start");

    memory_pressure_init();
    ahci_init();
    block_init();
    landlock_init();
    seccomp_init();
    appliance_config_init();

    /* Initialize VFS */
    printk("Initializing VFS...\n");
    vfs_init();
    printk("VFS initialized.\n\n");
    
    /* Initialize process manager */
    printk("Initializing process manager...\n");
    process_init();
    load_balance_init();
    sched_domain_init();
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
    sched_stats_init();
    sched_tune_init();
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
    fs_checksum_init();
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
    
    printk("Serial port ready.\n\n");
    
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
    framebuffer_init_multiboot(kernel_mb_info);
    fb_console_init();
    virtio_gpu_init();
    printk("Framebuffer initialized.\n\n");
    
    /* Initialize multimedia subsystem */
    printk("Initializing multimedia subsystem...\n");
    media_init();
    extern void video_core_setup_default(void);
    extern void codec_raw_init(void);
    video_core_setup_default();
    codec_raw_init();
    pc_speaker_init();
    printk("Multimedia subsystem initialized.\n\n");
    
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
    
    /* seccomp / block: initialized at extended_start */
    
    /* Initialize I/O schedulers */
    printk("Initializing I/O schedulers...\n");
    io_sched_init();
    printk("I/O schedulers initialized.\n\n");
    
    /* Initialize tmpfs */
    printk("Initializing tmpfs...\n");
    tmpfs_init();
    tmpfs_mount("/tmp", 10 * 1024 * 1024);
    appliance_config_save("/etc/appliance.conf");
    appliance_config_load("/etc/appliance.conf");
    ext2_mount("hda", "/");
    landlock_add_rule(0, "/etc/", LANDLOCK_ACCESS_FS_READ, false);
    printk("Tmpfs + config initialized.\n\n");
    
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
    ebpf_init();
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
    
    /* Initialize IO_URING */
    printk("Initializing IO_URING...\n");
    io_uring_init();
    printk("IO_URING initialized.\n\n");
    
    /* Initialize FUSE */
    printk("Initializing FUSE...\n");
    fuse_init();
    printk("FUSE initialized.\n\n");
    
    /* Initialize IOMMU */
    printk("Initializing IOMMU...\n");
    iommu_init();
    printk("IOMMU initialized.\n\n");
    
    /* Initialize energy management */
    printk("Initializing energy management...\n");
    energy_init();
    printk("Energy management initialized.\n\n");
    
    /* Initialize device tree */
    printk("Initializing device tree...\n");
    devicetree_init(NULL); /* Would pass DTB from boot */
    printk("Device tree initialized.\n\n");
    
    /* Initialize KASAN */
    printk("Initializing KASAN...\n");
    kasan_init();
    printk("KASAN initialized.\n\n");
    
    /* Initialize OverlayFS */
    printk("Initializing OverlayFS...\n");
    overlayfs_init();
    printk("OverlayFS initialized.\n\n");
    
    /* ========================================
     * SECURITY HARDENING - EXTREME MODE
     * ======================================== */
    
    /* Initialize secure random first (needed by others) */
    printk("Initializing secure random...\n");
    secure_random_init();
    printk("Secure random initialized.\n\n");
    
    /* Initialize security hardening */
    printk("Initializing security hardening...\n");
    security_hardening_init();
    printk("Security hardening initialized.\n\n");
    
    /* Enable kernel lockdown */
    printk("Enabling kernel lockdown...\n");
    security_enable_lockdown();
    printk("Kernel lockdown enabled.\n\n");
    
    /* Initialize TPM */
    printk("Initializing TPM...\n");
    tpm_init();
    printk("TPM initialized.\n\n");
    
    /* Initialize memory encryption */
    printk("Initializing memory encryption...\n");
    memory_encryption_init();
    if (memory_encryption_is_enabled()) {
        printk("Memory encryption enabled.\n\n");
    } else {
        printk("Memory encryption not available.\n\n");
    }
    
    /* Initialize MAC framework */
    printk("Initializing MAC framework...\n");
    mac_init();
    mac_enable(MAC_TYPE_SELINUX); /* Enable SELinux-like */
    printk("MAC framework initialized.\n\n");
    
    /* Initialize secure boot */
    printk("Initializing secure boot...\n");
    secure_boot_init();
    printk("Secure boot initialized.\n\n");
    
    /* Initialize ROP protection */
    printk("Initializing ROP protection...\n");
    rop_protection_init();
    printk("ROP protection initialized.\n\n");
    
    /* Final security summary */
    printk("========================================\n");
    printk("  SECURITY STATUS:\n");
    printk("  - SMEP: %s\n", security_is_enabled(SECURITY_SMEP_ENABLED) ? "ENABLED" : "DISABLED");
    printk("  - SMAP: %s\n", security_is_enabled(SECURITY_SMAP_ENABLED) ? "ENABLED" : "DISABLED");
    printk("  - KPTI: %s\n", security_is_enabled(SECURITY_KPTI_ENABLED) ? "ENABLED" : "DISABLED");
    printk("  - Retpoline: %s\n", security_is_enabled(SECURITY_RETPOLINE) ? "ENABLED" : "DISABLED");
    printk("  - CFI: %s\n", security_is_enabled(SECURITY_CFI_ENABLED) ? "ENABLED" : "DISABLED");
    printk("  - Stack Protector: %s\n", security_is_enabled(SECURITY_STACK_PROT) ? "ENABLED" : "DISABLED");
    printk("  - Kernel Lockdown: %s\n", security_is_enabled(SECURITY_LOCKDOWN) ? "ENABLED" : "DISABLED");
    printk("  - Memory Encryption: %s\n", memory_encryption_is_enabled() ? "ENABLED" : "DISABLED");
    printk("  - ROP Protection: %s\n", rop_protection_is_enabled() ? "ENABLED" : "DISABLED");
    printk("========================================\n\n");
    
    /* Initialize RDMA */
    printk("Initializing RDMA...\n");
    rdma_init();
    printk("RDMA initialized.\n\n");
    
    /* Initialize DPDK */
    printk("Initializing DPDK...\n");
    dpdk_init();
    printk("DPDK initialized.\n\n");
    
    /* Initialize XDP */
    printk("Initializing XDP...\n");
    xdp_init();
    printk("XDP initialized.\n\n");
    
    /* Initialize NVMe */
    printk("Initializing NVMe...\n");
    nvme_init();
    printk("NVMe initialized.\n\n");
    
    /* Initialize container runtime */
    printk("Initializing container runtime...\n");
    container_init();
    printk("Container runtime initialized.\n\n");
    
    /* Initialize checkpoint/restore */
    printk("Initializing checkpoint/restore...\n");
    checkpoint_init();
    printk("Checkpoint/restore initialized.\n\n");
    

    boot_profiler_mark("extended_done");
    g_extended_ready = true;
    printk("[init] extended subsystems ready\n");
}
