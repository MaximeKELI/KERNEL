#ifndef CPU_TOPOLOGY_H
#define CPU_TOPOLOGY_H

#include "types.h"

/* CPU topology levels */
#define TOPOLOGY_SMT     0
#define TOPOLOGY_CORE    1
#define TOPOLOGY_PACKAGE 2
#define TOPOLOGY_NODE    3

/* CPU topology structure */
typedef struct cpu_topology {
    u32 cpu_id;
    u32 smt_id;
    u32 core_id;
    u32 package_id;
    u32 node_id;
    u32 cache_levels;
    u32 cache_sizes[4];
    struct cpu_topology* next;
} cpu_topology_t;

/* Initialize CPU topology */
int cpu_topology_init(void);

/* Get CPU topology */
int cpu_topology_get(u32 cpu_id, cpu_topology_t* topology);

/* Get CPUs in same core */
int cpu_topology_get_siblings(u32 cpu_id, u32* siblings, u32 max_count);

/* Get CPUs in same package */
int cpu_topology_get_package_cpus(u32 package_id, u32* cpus, u32 max_count);

/* Get CPUs in same NUMA node */
int cpu_topology_get_node_cpus(u32 node_id, u32* cpus, u32 max_count);

#endif /* CPU_TOPOLOGY_H */
