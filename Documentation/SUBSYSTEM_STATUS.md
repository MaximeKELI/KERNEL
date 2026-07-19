# Subsystem reality map (REAL / PARTIAL / STUB)

Honest status of the kernel's subsystems, established during the "Rattraper Linux"
audit. The goal is to avoid relying on cosmetic code. Keep this file updated as
phases land.

Legend:
- REAL: substantive logic that actually does the work and is wired in.
- PARTIAL: real data structures/bookkeeping, but the core action (hardware I/O,
  real algorithm, enforcement) is a placeholder / no-op.
- STUB: cosmetic. Sets an `enabled` flag, prints "Initialized", `TODO`, no logic.

## Core (relied upon)

- Memory: PMM bitmap + per-frame refcount (REAL), `kmalloc` heap (REAL, sized
  from `_kernel_end`), slab (REAL), 4-level paging + map/unmap + huge-page split
  (REAL), COW machinery (REAL). Per-process CR3 switch (REAL, P1), `mm_struct` +
  VMA tree + page-fault dispatcher (COW/anon/stack) (REAL, P2), real
  `mmap`/`munmap`/`mprotect`/`brk` (REAL, P2).
- Scheduler: CFS with RB-tree + vruntime (REAL), timer preemption (REAL),
  context switch (REAL), kthreads `kthread_run/join/exit` (REAL). Single global
  runqueue; SMP not real (see P7).
- Process lifecycle: `fork` (COW clone, ring-3 resume frame), `exec` (teardown +
  SysV ABI stack argv/envp/auxv), `wait` (zombie reaping, exit status),
  per-process fd table (REAL, P3).
- Signals: real ring-3 delivery on syscall/IRQ return, `sigreturn`, default
  actions, `rt_sigaction/procmask`, cross-process `kill`, `SIGSEGV` on fault
  (REAL, P4).
- Syscalls: `SYSCALL/SYSRET` entry (REAL), ~43 calls, custom (non-Linux) ABI
  (Linux ABI renumber pending, P6).
- FS: VFS root is now real hierarchical **ramfs** (dirs, path resolution,
  create/mkdir/unlink/rmdir/getdents, growing files) (REAL, P5). ext2 READ
  (REAL) and ext2 WRITE (REAL against a RAM-backed image: bitmap alloc, dir
  entries, inode writeback, single/double indirect blocks; real disk backend
  deferred to P8). Pipes: in-memory ring buffer wired to the fd table (REAL, P5).
- Net: L2/L3/L4 stack (REAL-ish), drivers virtio-net / rtl8139 / e1000 (REAL).
- Userland: ring-3 exec of static ELF with argv/env/auxv (REAL), no libc yet (P6).

## Substantive extras (REAL / REAL-ish)

- `kernel/ai/` (heuristic engine that mutates scheduler state)
- `kernel/media/` (codec/audio/video logic)
- `kernel/bpf/ebpf.c` (small working interpreter)
- `kernel/io/epoll.c`, `kernel/block/blk_mq.c`, `kernel/net/multipath.c`,
  `kernel/net/adaptive_routing.c`, `kernel/security/{secure_random,lsm,seccomp}.c`

## PARTIAL (bookkeeping, core action is a placeholder)

security (selinux/apparmor always allow, ima hash is random), virt/kvm,
container, cgroup (limits stored, not enforced), namespace (no real isolation),
trace/{ftrace,kprobes}, drivers/{gpu,audio,usb}, net/{dpdk,xdp,rdma,kernel_bypass,
hw_offload,zero_copy,network_slicing}, block/{dm,lvm,md}, power/{thermal,suspend,
cpufreq_governor,energy}, numa, perf, rcu (no real grace period), io/{io_uring,
eventfd,timerfd,signalfd}, acpi, iommu, kexec, mm/{thp,reclaim,compaction,memcg,
overcommit,userfaultfd,hugepages}.

## STUB (cosmetic — DO NOT rely on)

- `kernel/power/cpufreq.c`, `cpuidle.c`, `powercap.c`, `runtime_pm.c`
- `kernel/block/bcache.c`, `bfq.c`, `thin.c`, `dm_crypt.c`
- `kernel/security/yama.c`, `stack_protection.c`, `hsm.c`
- `kernel/container/security.c`
- `kernel/perf/profiling.c`
- `kernel/trace/events.c`, `tracepoint.c`
- `kernel/fs/{ext4,xfs,btrfs,overlayfs,fuse}.c` (print-only; ext2 is the real one)
- `kernel/efi/efi.c` (mostly no-ops)

These are targeted for real implementation or clear quarantine in phase P8.
