#!/usr/bin/env python3
"""
Script pour générer les fichiers de base pour toutes les fonctionnalités
"""

import os

FEATURES = [
    # CPU Frequency
    ("kernel/power/cpufreq.c", "include/cpufreq.h", "cpufreq_init", "CPU Frequency Scaling"),
    
    # Filesystems
    ("kernel/fs/ext4.c", "include/ext4.h", "ext4_init", "EXT4 Journaling Filesystem"),
    
    # Network
    ("kernel/net/netfilter.c", "include/netfilter.h", "netfilter_init", "Netfilter Framework"),
    ("kernel/net/tc.c", "include/tc.h", "tc_init", "Traffic Control"),
    ("kernel/net/packet_sched.c", "include/packet_sched.h", "packet_sched_init", "Packet Scheduler"),
    ("kernel/net/stats.c", "include/net_stats.h", "net_stats_init", "Network Statistics"),
    ("kernel/net/multipath.c", "include/multipath.h", "multipath_init", "Network Multipath"),
    
    # Security
    ("kernel/security/landlock.c", "include/landlock.h", "landlock_init", "Landlock Security"),
    ("kernel/security/yama.c", "include/yama.h", "yama_init", "Yama Security Module"),
    ("kernel/security/stack_protection.c", "include/stack_protection.h", "stack_protection_init", "Stack Clash Protection"),
    ("kernel/security/hsm.c", "include/hsm.h", "hsm_init", "Hardware Security Module"),
    
    # Block
    ("kernel/block/blk_mq.c", "include/blk_mq.h", "blk_mq_init", "Block Multi-Queue"),
    ("kernel/block/bfq.c", "include/bfq.h", "bfq_init", "BFQ I/O Scheduler"),
    ("kernel/block/dm_crypt.c", "include/dm_crypt.h", "dm_crypt_init", "Device Mapper Crypt"),
    ("kernel/block/bcache.c", "include/bcache.h", "bcache_init", "Block Cache"),
    ("kernel/block/thin.c", "include/thin.h", "thin_init", "Thin Provisioning"),
    
    # Containers
    ("kernel/container/security.c", "include/container_security.h", "container_security_init", "Container Security Profiles"),
    
    # Debugging
    ("kernel/bpf/ebpf.c", "include/ebpf.h", "ebpf_init", "Extended BPF"),
    ("kernel/debug/kgdb.c", "include/kgdb.h", "kgdb_init", "Kernel Debugging Interface"),
    ("kernel/trace/tracepoint.c", "include/tracepoint.h", "tracepoint_init", "Kernel Tracepoints"),
    ("kernel/trace/events.c", "include/trace_events.h", "trace_events_init", "Kernel Event Tracing"),
    ("kernel/perf/profiling.c", "include/profiling.h", "profiling_init", "Kernel Profiling"),
    
    # Power
    ("kernel/power/cpuidle.c", "include/cpuidle.h", "cpuidle_init", "CPU Idle States"),
    ("kernel/power/runtime_pm.c", "include/runtime_pm.h", "runtime_pm_init", "Runtime Power Management"),
    ("kernel/power/powercap.c", "include/powercap.h", "powercap_init", "Power Capping"),
    
    # Drivers
    ("kernel/drivers/usb/usb_core.c", "include/usb.h", "usb_init", "USB Core"),
    ("kernel/drivers/net/ethernet.c", "include/ethernet.h", "ethernet_init", "Ethernet Drivers"),
    ("kernel/drivers/gpu/drm.c", "include/drm.h", "drm_init", "DRM Graphics"),
    ("kernel/drivers/audio/alsa.c", "include/alsa.h", "alsa_init", "ALSA Audio"),
    ("kernel/drivers/input/evdev.c", "include/evdev.h", "evdev_init", "Input Devices"),
    
    # Filesystem features
    ("kernel/fs/fscrypt.c", "include/fscrypt.h", "fscrypt_init", "Filesystem Encryption"),
    ("kernel/fs/snapshot.c", "include/snapshot.h", "snapshot_init", "Filesystem Snapshots"),
    ("kernel/fs/compression.c", "include/fs_compression.h", "fs_compression_init", "Filesystem Compression"),
    ("kernel/fs/dedupe.c", "include/dedupe.h", "dedupe_init", "Filesystem Deduplication"),
    ("kernel/fs/checksum.c", "include/fs_checksum.h", "fs_checksum_init", "Filesystem Checksumming"),
    
    # Scheduler
    ("kernel/process/sched_stats.c", "include/sched_stats.h", "sched_stats_init", "Scheduler Statistics"),
    ("kernel/process/sched_tune.c", "include/sched_tune.h", "sched_tune_init", "Scheduler Tuning"),
    
    # Memory
    ("kernel/memory/bandwidth_ctrl.c", "include/memory_bandwidth_ctrl.h", "memory_bandwidth_ctrl_init", "Memory Bandwidth Controller"),
]

def generate_c_file(path, init_func, description):
    dirname = os.path.dirname(path)
    os.makedirs(dirname, exist_ok=True)
    
    basename = os.path.basename(path).replace('.c', '')
    
    content = f'''#include "{basename}.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* {description} */
static bool {basename}_enabled = false;
static spinlock_t {basename}_lock = SPINLOCK_INIT;

void {init_func}(void) {{
    {basename}_enabled = true;
    printk("[{description}] Initialized\\n");
}}

/* TODO: Implement {description} functionality */
'''
    
    with open(path, 'w') as f:
        f.write(content)
    
    print(f"Created: {path}")

def generate_h_file(path, init_func, description):
    dirname = os.path.dirname(path)
    os.makedirs(dirname, exist_ok=True)
    
    basename = os.path.basename(path).replace('.h', '').upper()
    basename = basename.replace('/', '_')
    
    content = f'''#ifndef {basename}_H
#define {basename}_H

#include "types.h"

/* {description} */

/* Initialize {description} */
void {init_func}(void);

/* TODO: Add function declarations */

#endif /* {basename}_H */
'''
    
    with open(path, 'w') as f:
        f.write(content)
    
    print(f"Created: {path}")

def main():
    print("Generating feature files...")
    print("=" * 50)
    
    for c_file, h_file, init_func, desc in FEATURES:
        generate_c_file(c_file, init_func, desc)
        generate_h_file(h_file, init_func, desc)
    
    print("=" * 50)
    print(f"Generated {len(FEATURES)} feature pairs")

if __name__ == "__main__":
    main()
