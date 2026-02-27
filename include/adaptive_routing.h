#ifndef ADAPTIVE_ROUTING_H
#define ADAPTIVE_ROUTING_H

#include "types.h"
#include "net.h"
#include "route.h"

/* Initialize adaptive routing */
void adaptive_routing_init(void);

/* Add adaptive route with multiple paths */
int adaptive_routing_add_route(ip_addr_t dst, ip_addr_t netmask,
                               route_entry_t** paths, u32 num_paths);

/* Select best path based on latency */
route_entry_t* adaptive_routing_select_path(ip_addr_t dst);

/* Update latency for path */
void adaptive_routing_update_latency(ip_addr_t dst, u32 path_index, u64 latency);

/* Get latency statistics */
void adaptive_routing_get_stats(ip_addr_t dst, u32 path_index,
                                 u64* avg_latency, u64* min_latency,
                                 u64* max_latency, u64* packet_count);

/* Enable/disable adaptive routing */
void adaptive_routing_enable(void);
void adaptive_routing_disable(void);
bool adaptive_routing_is_enabled(void);

#endif /* ADAPTIVE_ROUTING_H */
