#include "memory_bandwidth.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

#define MAX_NODES 8

static memory_bandwidth_stats_t node_stats[MAX_NODES];
static u64 bandwidth_limits[MAX_NODES] = {0};
static spinlock_t bandwidth_lock = SPINLOCK_INIT;

int memory_bandwidth_init(void) {
    memset(node_stats, 0, sizeof(node_stats));
    memset(bandwidth_limits, 0, sizeof(bandwidth_limits));
    DEBUG_INFO("%s", "Memory bandwidth monitoring initialized");
    return 0;
}

int memory_bandwidth_get_stats(u32 node_id, memory_bandwidth_stats_t* stats) {
    VALIDATE_PTR_RET(stats, -1);
    
    if (node_id >= MAX_NODES) {
        return -1;
    }
    
    spinlock_lock(&bandwidth_lock);
    *stats = node_stats[node_id];
    
    /* Calculate bandwidth in MB/s */
    u64 total_bytes = node_stats[node_id].read_bytes + node_stats[node_id].write_bytes;
    node_stats[node_id].bandwidth_mbps = total_bytes / (1024 * 1024); /* Simplified */
    
    /* Calculate utilization */
    if (bandwidth_limits[node_id] > 0) {
        node_stats[node_id].utilization_percent = 
            (u32)((node_stats[node_id].bandwidth_mbps * 100) / bandwidth_limits[node_id]);
    } else {
        node_stats[node_id].utilization_percent = 0;
    }
    
    *stats = node_stats[node_id];
    spinlock_unlock(&bandwidth_lock);
    
    return 0;
}

u64 memory_bandwidth_get_total(u32 node_id) {
    if (node_id >= MAX_NODES) {
        return 0;
    }
    
    spinlock_lock(&bandwidth_lock);
    u64 total = node_stats[node_id].read_bytes + node_stats[node_id].write_bytes;
    spinlock_unlock(&bandwidth_lock);
    
    return total;
}

u32 memory_bandwidth_get_utilization(u32 node_id) {
    if (node_id >= MAX_NODES) {
        return 0;
    }
    
    spinlock_lock(&bandwidth_lock);
    u32 util = node_stats[node_id].utilization_percent;
    spinlock_unlock(&bandwidth_lock);
    
    return util;
}

int memory_bandwidth_set_limit(u32 node_id, u64 limit_mbps) {
    if (node_id >= MAX_NODES) {
        return -1;
    }
    
    spinlock_lock(&bandwidth_lock);
    bandwidth_limits[node_id] = limit_mbps;
    spinlock_unlock(&bandwidth_lock);
    
    DEBUG_INFO("Memory bandwidth limit set: node=%u, limit=%llu MB/s", 
              node_id, (unsigned long long)limit_mbps);
    return 0;
}

/* Internal function to update statistics */
void memory_bandwidth_update(u32 node_id, u64 read_bytes, u64 write_bytes) {
    if (node_id >= MAX_NODES) {
        return;
    }
    
    spinlock_lock(&bandwidth_lock);
    node_stats[node_id].read_bytes += read_bytes;
    node_stats[node_id].write_bytes += write_bytes;
    node_stats[node_id].read_ops++;
    node_stats[node_id].write_ops++;
    spinlock_unlock(&bandwidth_lock);
}
