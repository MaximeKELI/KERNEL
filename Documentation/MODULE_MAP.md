# Cartographie des modules du noyau

Classification automatique (heuristique) :

- **full** : logique substantielle, plusieurs fonctions
- **partial** : code présent mais TODO / fonctionnalités incomplètes
- **stub** : surtout `*_init()` + message ou squelette minimal

**Totaux** : 100 complets, 71 partiels, 29 stubs (200 fichiers `.c` sous `kernel/`).

## `(root)/`

### full (1)

- `kernel.c (629 lines)`

### partial (1)

- `spinlock.c (38 lines)`

### stub (1)

- `debug.c (7 lines)`

## `acpi/`

### partial (1)

- `acpi/acpi.c (89 lines)`

## `ai/`

### full (3)

- `ai/ai_monitor.c (164 lines)`
- `ai/ai_optimizer.c (237 lines)`
- `ai/ai_sysfs.c (188 lines)`

### partial (1)

- `ai/ai_manager.c (53 lines)`

## `block/`

### full (3)

- `block/dm.c (168 lines)`
- `block/lvm.c (271 lines)`
- `block/md.c (168 lines)`

### partial (2)

- `block/block.c (91 lines)`
- `block/io_sched.c (57 lines)`

### stub (5)

- `block/bcache.c (16 lines)`
- `block/bfq.c (16 lines)`
- `block/blk_mq.c (16 lines)`
- `block/dm_crypt.c (16 lines)`
- `block/thin.c (16 lines)`

## `bpf/`

### full (1)

- `bpf/bpf.c (102 lines)`

### stub (1)

- `bpf/ebpf.c (16 lines)`

## `cache/`

### full (1)

- `cache/cache.c (221 lines)`

## `cgroup/`

### full (1)

- `cgroup/cgroup.c (102 lines)`

## `checkpoint/`

### partial (1)

- `checkpoint/checkpoint.c (106 lines)`

## `container/`

### full (1)

- `container/container.c (147 lines)`

### stub (1)

- `container/security.c (16 lines)`

## `debug/`

### stub (1)

- `debug/kgdb.c (16 lines)`

## `device/`

### full (1)

- `device/device.c (110 lines)`

## `devicetree/`

### partial (1)

- `devicetree/devicetree.c (61 lines)`

## `drivers/`

### full (9)

- `drivers/ata.c (109 lines)`
- `drivers/audio/alsa.c (196 lines)`
- `drivers/gpu/drm.c (187 lines)`
- `drivers/input/evdev.c (212 lines)`
- `drivers/net/ethernet.c (200 lines)`
- `drivers/nvme.c (104 lines)`
- `drivers/pci.c (122 lines)`
- `drivers/usb/usb_core.c (212 lines)`
- `drivers/vga.c (103 lines)`

### partial (4)

- `drivers/framebuffer.c (56 lines)`
- `drivers/keyboard.c (66 lines)`
- `drivers/serial.c (55 lines)`
- `drivers/timer.c (63 lines)`

## `efi/`

### partial (1)

- `efi/efi.c (27 lines)`

## `elf/`

### partial (1)

- `elf/elf.c (84 lines)`

## `fs/`

### full (10)

- `fs/acl.c (178 lines)`
- `fs/dentry.c (177 lines)`
- `fs/encrypted_fs.c (120 lines)`
- `fs/ext2.c (171 lines)`
- `fs/fanotify.c (211 lines)`
- `fs/inode.c (114 lines)`
- `fs/inotify.c (213 lines)`
- `fs/procfs.c (126 lines)`
- `fs/quota.c (172 lines)`
- `fs/xattr.c (183 lines)`

### partial (9)

- `fs/btrfs.c (95 lines)`
- `fs/devtmpfs.c (73 lines)`
- `fs/dnotify.c (96 lines)`
- `fs/fuse.c (39 lines)`
- `fs/overlayfs.c (28 lines)`
- `fs/sysfs.c (53 lines)`
- `fs/tmpfs.c (55 lines)`
- `fs/vfs.c (92 lines)`
- `fs/xfs.c (47 lines)`

### stub (6)

- `fs/checksum.c (16 lines)`
- `fs/compression.c (16 lines)`
- `fs/dedupe.c (16 lines)`
- `fs/ext4.c (16 lines)`
- `fs/fscrypt.c (16 lines)`
- `fs/snapshot.c (16 lines)`

## `hotplug/`

### partial (1)

- `hotplug/hotplug.c (95 lines)`

## `interrupt/`

### full (2)

- `interrupt/idt.c (176 lines)`
- `interrupt/pic.c (102 lines)`

### partial (1)

- `interrupt/exceptions.c (71 lines)`

## `io/`

### full (1)

- `io/timerfd.c (131 lines)`

### partial (4)

- `io/epoll.c (80 lines)`
- `io/eventfd.c (101 lines)`
- `io/io_uring.c (86 lines)`
- `io/signalfd.c (116 lines)`

## `iommu/`

### partial (1)

- `iommu/iommu.c (72 lines)`

## `ipc/`

### full (3)

- `ipc/futex.c (183 lines)`
- `ipc/ipc.c (259 lines)`
- `ipc/sysv_ipc.c (143 lines)`

### partial (1)

- `ipc/robust_futex.c (120 lines)`

## `kexec/`

### partial (1)

- `kexec/kexec.c (46 lines)`

## `kthread/`

### partial (1)

- `kthread/kthread.c (89 lines)`

## `livepatch/`

### partial (1)

- `livepatch/livepatch.c (84 lines)`

## `log/`

### partial (1)

- `log/log.c (96 lines)`

## `media/`

### full (6)

- `media/audio_core.c (406 lines)`
- `media/av_sync.c (255 lines)`
- `media/codec.c (910 lines)`
- `media/media.c (211 lines)`
- `media/streaming.c (420 lines)`
- `media/video_core.c (367 lines)`

### partial (1)

- `media/media_modern.c (326 lines)`

## `memory/`

### full (13)

- `memory/heap.c (183 lines)`
- `memory/kswapd.c (108 lines)`
- `memory/memcg.c (173 lines)`
- `memory/memory_hotplug.c (171 lines)`
- `memory/mpk.c (111 lines)`
- `memory/oom_killer.c (118 lines)`
- `memory/overcommit.c (111 lines)`
- `memory/pmm.c (113 lines)`
- `memory/pressure.c (107 lines)`
- `memory/slab.c (156 lines)`
- `memory/thp.c (100 lines)`
- `memory/userfaultfd.c (146 lines)`
- `memory/vmm.c (151 lines)`

### partial (4)

- `memory/compaction.c (101 lines)`
- `memory/defrag.c (86 lines)`
- `memory/hugepages.c (93 lines)`
- `memory/reclaim.c (39 lines)`

### stub (1)

- `memory/bandwidth_ctrl.c (18 lines)`

## `module/`

### full (1)

- `module/module.c (150 lines)`

## `namespace/`

### partial (1)

- `namespace/namespace.c (96 lines)`

## `net/`

### full (16)

- `net/adaptive_routing.c (212 lines)`
- `net/arp.c (182 lines)`
- `net/hw_offload.c (212 lines)`
- `net/icmp.c (133 lines)`
- `net/ip.c (238 lines)`
- `net/kernel_bypass.c (315 lines)`
- `net/net.c (157 lines)`
- `net/netfilter.c (176 lines)`
- `net/network_slicing.c (242 lines)`
- `net/packet_sched.c (153 lines)`
- `net/route.c (150 lines)`
- `net/skbuff.c (137 lines)`
- `net/tc.c (164 lines)`
- `net/tcp.c (644 lines)`
- `net/udp.c (316 lines)`
- `net/zero_copy.c (157 lines)`

### partial (5)

- `net/dpdk.c (90 lines)`
- `net/multipath.c (115 lines)`
- `net/rdma.c (74 lines)`
- `net/stats.c (57 lines)`
- `net/xdp.c (76 lines)`

## `numa/`

### partial (2)

- `numa/memory_bandwidth.c (99 lines)`
- `numa/numa.c (89 lines)`

## `perf/`

### partial (1)

- `perf/perf.c (68 lines)`

### stub (1)

- `perf/profiling.c (16 lines)`

## `power/`

### full (4)

- `power/cpufreq_governor.c (148 lines)`
- `power/energy.c (118 lines)`
- `power/suspend.c (143 lines)`
- `power/thermal.c (167 lines)`

### stub (4)

- `power/cpufreq.c (16 lines)`
- `power/cpuidle.c (16 lines)`
- `power/powercap.c (16 lines)`
- `power/runtime_pm.c (16 lines)`

## `process/`

### full (6)

- `process/fork.c (128 lines)`
- `process/pi_mutex.c (127 lines)`
- `process/sched_domain.c (188 lines)`
- `process/sched_rt.c (249 lines)`
- `process/scheduler.c (169 lines)`
- `process/scheduler_cfs.c (259 lines)`

### partial (2)

- `process/load_balance.c (150 lines)`
- `process/sched_deadline.c (85 lines)`

### stub (2)

- `process/sched_stats.c (16 lines)`
- `process/sched_tune.c (16 lines)`

## `rcu/`

### partial (1)

- `rcu/rcu.c (66 lines)`

## `security/`

### full (6)

- `security/apparmor.c (139 lines)`
- `security/capabilities.c (102 lines)`
- `security/ima.c (120 lines)`
- `security/seccomp.c (126 lines)`
- `security/security_hardening.c (105 lines)`
- `security/selinux.c (121 lines)`

### partial (12)

- `security/audit.c (102 lines)`
- `security/evm.c (97 lines)`
- `security/kasan.c (87 lines)`
- `security/kaslr.c (51 lines)`
- `security/kspp.c (58 lines)`
- `security/lsm.c (87 lines)`
- `security/mac.c (81 lines)`
- `security/memory_encryption.c (78 lines)`
- `security/rop_protection.c (67 lines)`
- `security/secure_boot.c (88 lines)`
- `security/secure_random.c (93 lines)`
- `security/tpm.c (87 lines)`

### stub (4)

- `security/hsm.c (16 lines)`
- `security/landlock.c (16 lines)`
- `security/stack_protection.c (16 lines)`
- `security/yama.c (16 lines)`

## `signal/`

### full (1)

- `signal/signal.c (131 lines)`

## `smp/`

### full (3)

- `smp/cache_qos.c (138 lines)`
- `smp/cpu_isolation.c (164 lines)`
- `smp/cpu_topology.c (128 lines)`

### partial (2)

- `smp/membarrier.c (77 lines)`
- `smp/smp.c (84 lines)`

## `syscall/`

### full (1)

- `syscall/syscall.c (156 lines)`

## `test/`

### full (4)

- `test/test.c (136 lines)`
- `test/tests_ai.c (150 lines)`
- `test/tests_complete.c (476 lines)`
- `test/tests_network.c (227 lines)`

### partial (2)

- `test/tests_memory.c (68 lines)`
- `test/tests_scheduler.c (29 lines)`

## `timer/`

### partial (1)

- `timer/hrtimer.c (106 lines)`

## `trace/`

### partial (2)

- `trace/ftrace.c (84 lines)`
- `trace/kprobes.c (58 lines)`

### stub (2)

- `trace/events.c (16 lines)`
- `trace/tracepoint.c (26 lines)`

## `virt/`

### full (1)

- `virt/kvm.c (104 lines)`

## `watchdog/`

### partial (1)

- `watchdog/watchdog.c (71 lines)`

## `workqueue/`

### full (1)

- `workqueue/workqueue.c (119 lines)`
