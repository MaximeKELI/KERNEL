#ifndef NET_STATS_H
#define NET_STATS_H

#include "types.h"

/* Network statistics */
typedef struct {
    u64 rx_packets;
    u64 tx_packets;
    u64 rx_bytes;
    u64 tx_bytes;
    u64 rx_errors;
    u64 tx_errors;
    u64 rx_dropped;
    u64 tx_dropped;
    u64 collisions;
    u64 multicast;
} net_stats_t;

/* Initialize network statistics */
void net_stats_init(void);

/* Update statistics */
void net_stats_update_rx(u64 bytes);
void net_stats_update_tx(u64 bytes);
void net_stats_update_rx_error(void);
void net_stats_update_tx_error(void);

/* Get/reset statistics */
void net_stats_get(net_stats_t* stats);
void net_stats_reset(void);

#endif /* NET_STATS_H */
