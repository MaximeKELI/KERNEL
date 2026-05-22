#include "cpu_topology.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "io.h"
#include "string.h"

#define MAX_CPUS 64

static cpu_topology_t cpu_topologies[MAX_CPUS];
static spinlock_t topology_lock = SPINLOCK_INIT;

static void cpu_topology_detect(u32 cpu_id) {
    cpu_topology_t* topo = &cpu_topologies[cpu_id];
    topo->cpu_id = cpu_id;
    
    /* Detect topology via CPUID */
    u32 eax, ebx, ecx, edx;
    
    /* Get APIC ID */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    u32 apic_id = (ebx >> 24) & 0xFF;
    
    /* Get extended topology */
    cpuid(0xB, &eax, &ebx, &ecx, &edx);
    topo->smt_id = eax & 0xFFFF;
    topo->core_id = (eax >> 16) & 0xFFFF;
    
    /* Package ID from APIC */
    topo->package_id = apic_id / 8; /* Simplified */
    topo->node_id = topo->package_id; /* Assume 1:1 mapping */
    
    /* Cache information */
    topo->cache_levels = 0;
    for (u32 i = 0; i < 4; i++) {
        cpuid(4, &eax, &ebx, &ecx, &edx);
        if ((eax & 0x1F) == 0) break; /* No more cache levels */
        
        u32 cache_size = ((ebx >> 22) + 1) * ((ebx & 0x7FF) + 1) *
                         (((ebx >> 12) & 0x3FF) + 1) * (ecx + 1);
        topo->cache_sizes[topo->cache_levels++] = cache_size;
    }
}

int cpu_topology_init(void) {
    memset(cpu_topologies, 0, sizeof(cpu_topologies));
    
    /* Detect topology for all CPUs */
    for (u32 i = 0; i < MAX_CPUS; i++) {
        cpu_topology_detect(i);
    }
    
    DEBUG_INFO("%s", "CPU topology initialized");
    return 0;
}

int cpu_topology_get(u32 cpu_id, cpu_topology_t* topology) {
    VALIDATE_PTR_RET(topology, -1);
    
    if (cpu_id >= MAX_CPUS) {
        return -1;
    }
    
    spinlock_lock(&topology_lock);
    *topology = cpu_topologies[cpu_id];
    spinlock_unlock(&topology_lock);
    
    return 0;
}

int cpu_topology_get_siblings(u32 cpu_id, u32* siblings, u32 max_count) {
    VALIDATE_PTR_RET(siblings, -1);
    
    if (cpu_id >= MAX_CPUS) {
        return -1;
    }
    
    cpu_topology_t topo;
    if (cpu_topology_get(cpu_id, &topo) != 0) {
        return -1;
    }
    
    u32 count = 0;
    for (u32 i = 0; i < MAX_CPUS && count < max_count; i++) {
        cpu_topology_t other;
        if (cpu_topology_get(i, &other) == 0) {
            if (other.core_id == topo.core_id && other.cpu_id != cpu_id) {
                siblings[count++] = other.cpu_id;
            }
        }
    }
    
    return (int)count;
}

int cpu_topology_get_package_cpus(u32 package_id, u32* cpus, u32 max_count) {
    VALIDATE_PTR_RET(cpus, -1);
    
    u32 count = 0;
    for (u32 i = 0; i < MAX_CPUS && count < max_count; i++) {
        cpu_topology_t topo;
        if (cpu_topology_get(i, &topo) == 0) {
            if (topo.package_id == package_id) {
                cpus[count++] = topo.cpu_id;
            }
        }
    }
    
    return (int)count;
}

int cpu_topology_get_node_cpus(u32 node_id, u32* cpus, u32 max_count) {
    VALIDATE_PTR_RET(cpus, -1);
    
    u32 count = 0;
    for (u32 i = 0; i < MAX_CPUS && count < max_count; i++) {
        cpu_topology_t topo;
        if (cpu_topology_get(i, &topo) == 0) {
            if (topo.node_id == node_id) {
                cpus[count++] = topo.cpu_id;
            }
        }
    }
    
    return (int)count;
}
