#ifndef CACHE_QOS_H
#define CACHE_QOS_H

#include "types.h"

/* Cache QoS classes */
#define CACHE_QOS_CLASS_L3 0
#define CACHE_QOS_CLASS_MB 1

/* Cache QoS monitoring */
typedef struct cache_qos_monitor {
    u32 class_id;
    u64 llc_occupancy;
    u64 memory_bandwidth;
    u32 utilization_percent;
} cache_qos_monitor_t;

/* Initialize Cache QoS */
int cache_qos_init(void);

/* Set cache allocation for process */
int cache_qos_set_allocation(u64 pid, u32 class_id, u64 max_occupancy);

/* Get cache allocation */
int cache_qos_get_allocation(u64 pid, u32* class_id, u64* max_occupancy);

/* Monitor cache usage */
int cache_qos_monitor(u64 pid, cache_qos_monitor_t* monitor);

/* Set memory bandwidth limit */
int cache_qos_set_bandwidth_limit(u64 pid, u64 limit_mbps);

#endif /* CACHE_QOS_H */
