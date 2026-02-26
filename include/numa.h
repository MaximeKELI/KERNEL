#ifndef NUMA_H
#define NUMA_H

#include "types.h"

/* NUMA node structure */
typedef struct numa_node {
    u32 node_id;
    u64 memory_start;
    u64 memory_end;
    u32 cpu_count;
    u32* cpu_ids;
    struct numa_node* next;
} numa_node_t;

/* Initialize NUMA */
void numa_init(void);

/* Get NUMA node for CPU */
numa_node_t* numa_node_of_cpu(u32 cpu_id);

/* Get NUMA node for address */
numa_node_t* numa_node_of_addr(void* addr);

/* Allocate on specific node */
void* numa_alloc_on_node(size_t size, u32 node_id);

/* Get node distance */
u32 numa_distance(u32 node1, u32 node2);

/* Get number of nodes */
u32 numa_node_count(void);

#endif /* NUMA_H */
