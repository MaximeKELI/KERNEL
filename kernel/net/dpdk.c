#include "dpdk.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

static dpdk_port_t* dpdk_ports = NULL;
static spinlock_t dpdk_lock = SPINLOCK_INIT;

void dpdk_init(void) {
    DEBUG_INFO("DPDK (Data Plane Development Kit) initialized");
}

int dpdk_port_init(u32 port_id, u32 nb_rx_queues, u32 nb_tx_queues) {
    dpdk_port_t* port = (dpdk_port_t*)kzalloc(sizeof(dpdk_port_t));
    if (!port) {
        DEBUG_ERROR("Failed to allocate DPDK port");
        return -1;
    }
    
    port->port_id = port_id;
    port->nb_rx_queues = nb_rx_queues;
    port->nb_tx_queues = nb_tx_queues;
    
    /* Allocate rings */
    size_t ring_size = 4096 * sizeof(void*);
    port->rx_rings = vmm_alloc_pages((ring_size + PAGE_SIZE - 1) / PAGE_SIZE);
    port->tx_rings = vmm_alloc_pages((ring_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!port->rx_rings || !port->tx_rings) {
        if (port->rx_rings) vmm_free_pages(port->rx_rings, (ring_size + PAGE_SIZE - 1) / PAGE_SIZE);
        if (port->tx_rings) vmm_free_pages(port->tx_rings, (ring_size + PAGE_SIZE - 1) / PAGE_SIZE);
        kfree(port);
        return -1;
    }
    
    spinlock_lock(&dpdk_lock);
    port->next = dpdk_ports;
    dpdk_ports = port;
    spinlock_unlock(&dpdk_lock);
    
    DEBUG_INFO("DPDK port initialized: port_id=%u, rx_queues=%u, tx_queues=%u",
               port_id, nb_rx_queues, nb_tx_queues);
    return 0;
}

u16 dpdk_rx_burst(u32 port_id, u32 queue_id, dpdk_mbuf_t** pkts, u16 nb_pkts) {
    (void)port_id;
    (void)queue_id;
    (void)pkts;
    (void)nb_pkts;
    
    /* Would receive packets */
    return 0;
}

u16 dpdk_tx_burst(u32 port_id, u32 queue_id, dpdk_mbuf_t** pkts, u16 nb_pkts) {
    (void)port_id;
    (void)queue_id;
    (void)pkts;
    (void)nb_pkts;
    
    /* Would transmit packets */
    return 0;
}

dpdk_mbuf_t* dpdk_alloc_mbuf(void) {
    dpdk_mbuf_t* mbuf = (dpdk_mbuf_t*)kzalloc(sizeof(dpdk_mbuf_t));
    if (mbuf) {
        mbuf->buf_addr = vmm_alloc_pages(1);
        if (!mbuf->buf_addr) {
            kfree(mbuf);
            return NULL;
        }
    }
    return mbuf;
}

void dpdk_free_mbuf(dpdk_mbuf_t* mbuf) {
    if (!mbuf) return;
    
    if (mbuf->buf_addr) {
        vmm_free_pages(mbuf->buf_addr, 1);
    }
    kfree(mbuf);
}
