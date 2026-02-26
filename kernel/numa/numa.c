#include "numa.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "smp.h"

static numa_node_t* numa_nodes = NULL;
static u32 numa_node_count_val = 1;
static spinlock_t numa_lock = SPINLOCK_INIT;

void numa_init(void) {
    /* Create default node 0 */
    numa_node_t* node = (numa_node_t*)kzalloc(sizeof(numa_node_t));
    if (node) {
        node->node_id = 0;
        node->memory_start = 0;
        node->memory_end = 512 * 1024 * 1024; /* 512MB */
        node->cpu_count = 1;
        node->cpu_ids = (u32*)kzalloc(sizeof(u32));
        if (node->cpu_ids) {
            node->cpu_ids[0] = 0;
        }
        
        numa_nodes = node;
        numa_node_count_val = 1;
    }
    
    DEBUG_INFO("NUMA (Non-Uniform Memory Access) initialized: %u node(s)", numa_node_count_val);
}

numa_node_t* numa_node_of_cpu(u32 cpu_id) {
    spinlock_lock(&numa_lock);
    
    numa_node_t* node = numa_nodes;
    while (node) {
        for (u32 i = 0; i < node->cpu_count; i++) {
            if (node->cpu_ids[i] == cpu_id) {
                spinlock_unlock(&numa_lock);
                return node;
            }
        }
        node = node->next;
    }
    
    spinlock_unlock(&numa_lock);
    return numa_nodes; /* Default to node 0 */
}

numa_node_t* numa_node_of_addr(void* addr) {
    u64 addr_val = (u64)addr;
    
    spinlock_lock(&numa_lock);
    
    numa_node_t* node = numa_nodes;
    while (node) {
        if (addr_val >= node->memory_start && addr_val < node->memory_end) {
            spinlock_unlock(&numa_lock);
            return node;
        }
        node = node->next;
    }
    
    spinlock_unlock(&numa_lock);
    return numa_nodes; /* Default to node 0 */
}

void* numa_alloc_on_node(size_t size, u32 node_id) {
    numa_node_t* node = numa_nodes;
    
    while (node) {
        if (node->node_id == node_id) {
            /* Allocate from node's memory */
            return vmm_alloc_pages((size + PAGE_SIZE - 1) / PAGE_SIZE);
        }
        node = node->next;
    }
    
    return NULL;
}

u32 numa_distance(u32 node1, u32 node2) {
    if (node1 == node2) return 10; /* Same node */
    return 20; /* Different nodes */
}

u32 numa_node_count(void) {
    return numa_node_count_val;
}
