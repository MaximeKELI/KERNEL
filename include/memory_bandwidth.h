#ifndef MEMORY_BANDWIDTH_H
#define MEMORY_BANDWIDTH_H

#include "types.h"

/* Memory bandwidth statistics */
typedef struct memory_bandwidth_stats {
    u64 read_bytes;
    u64 write_bytes;
    u64 read_ops;
    u64 write_ops;
    u64 bandwidth_mbps;
    u32 utilization_percent;
} memory_bandwidth_stats_t;

/* Initialize memory bandwidth monitoring */
int memory_bandwidth_init(void);

/* Get bandwidth statistics */
int memory_bandwidth_get_stats(u32 node_id, memory_bandwidth_stats_t* stats);

/* Get total bandwidth */
u64 memory_bandwidth_get_total(u32 node_id);

/* Get utilization */
u32 memory_bandwidth_get_utilization(u32 node_id);

/* Set bandwidth limit */
int memory_bandwidth_set_limit(u32 node_id, u64 limit_mbps);

#endif /* MEMORY_BANDWIDTH_H */
