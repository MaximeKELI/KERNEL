#ifndef DPDK_H
#define DPDK_H

#include "types.h"

/* DPDK port */
typedef struct dpdk_port {
    u32 port_id;
    void* rx_rings;
    void* tx_rings;
    u32 nb_rx_queues;
    u32 nb_tx_queues;
    struct dpdk_port* next;
} dpdk_port_t;

/* DPDK mbuf (packet buffer) */
typedef struct dpdk_mbuf {
    void* buf_addr;
    u16 data_len;
    u16 pkt_len;
    u16 nb_segs;
    struct dpdk_mbuf* next;
} dpdk_mbuf_t;

/* Initialize DPDK */
void dpdk_init(void);

/* Initialize port */
int dpdk_port_init(u32 port_id, u32 nb_rx_queues, u32 nb_tx_queues);

/* Receive packets */
u16 dpdk_rx_burst(u32 port_id, u32 queue_id, dpdk_mbuf_t** pkts, u16 nb_pkts);

/* Transmit packets */
u16 dpdk_tx_burst(u32 port_id, u32 queue_id, dpdk_mbuf_t** pkts, u16 nb_pkts);

/* Allocate mbuf */
dpdk_mbuf_t* dpdk_alloc_mbuf(void);

/* Free mbuf */
void dpdk_free_mbuf(dpdk_mbuf_t* mbuf);

#endif /* DPDK_H */
