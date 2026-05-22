#include "net.h"
#include "skbuff.h"
#include "ip.h"
#include "tcp.h"
#include "udp.h"
#include "icmp.h"
#include "arp.h"
#include "route.h"
#include "netfilter.h"
#include "ethernet.h"
#include "net_stats.h"
#include "tc.h"
#include "packet_sched.h"
#include "multipath.h"
#include "kernel_bypass.h"
#include "hw_offload.h"
#include "zero_copy.h"
#include "adaptive_routing.h"
#include "network_slicing.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

/* Global network statistics for AI monitoring */
u64 global_net_tx_bytes = 0;
u64 global_net_rx_bytes = 0;
u64 global_net_tx_packets = 0;
u64 global_net_rx_packets = 0;

netif_t* netif_list = NULL;
static socket_t* socket_list = NULL;
static spinlock_t net_lock = SPINLOCK_INIT;

void net_init(void) {
    /* Initialize network subsystems */
    skb_init();
    ip_init();
    tcp_init();
    udp_init();
    icmp_init();
    arp_init();
    route_init();
    netfilter_init();
    net_stats_init();
    tc_init();
    packet_sched_init();
    multipath_init();
    
    /* Initialize advanced networking features */
    kernel_bypass_init();
    hw_offload_init();
    zero_copy_init();
    adaptive_routing_init();
    network_slicing_init();
    
    DEBUG_INFO("Networking stack initialized");
    printk("[Net] Complete networking infrastructure initialized\n");
}

socket_t* socket_create(int domain, int type, int protocol) {
    /* Validate parameters */
    VALIDATE_RANGE(domain, 0, 255);
    VALIDATE_RANGE(type, 0, 255);
    VALIDATE_RANGE(protocol, 0, 255);
    
    socket_t* sock = (socket_t*)kzalloc(sizeof(socket_t));
    if (!sock) {
        DEBUG_ERROR("Failed to allocate socket");
        return NULL;
    }
    
    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;
    
    spinlock_lock(&net_lock);
    sock->private_data = (void*)socket_list;
    socket_list = sock;
    spinlock_unlock(&net_lock);
    
    DEBUG_INFO("Socket created: domain=%d, type=%d, protocol=%d",
               domain, type, protocol);
    return sock;
}

int netif_register(netif_t* iface) {
    VALIDATE_PTR(iface);
    
    spinlock_lock(&net_lock);
    iface->next = netif_list;
    netif_list = iface;
    spinlock_unlock(&net_lock);
    
    DEBUG_INFO("Network interface registered: %s", iface->name);
    return 0;
}

int net_send_packet(netif_t* iface, const void* data, size_t len) {
    VALIDATE_PTR(iface);
    VALIDATE_PTR(data);
    VALIDATE_RANGE(len, 0, 65535); /* Max Ethernet frame size */
    
    /* Update network statistics */
    global_net_tx_bytes += len;
    global_net_tx_packets++;
    
    (void)iface;
    (void)data;
    (void)len;
    /* Would send via network driver */
    return 0;
}

int net_recv_packet(netif_t* iface, void* data, size_t* len) {
    VALIDATE_PTR(iface);
    VALIDATE_PTR(data);
    VALIDATE_PTR(len);
    
    /* Update network statistics */
    if (len && *len > 0) {
        global_net_rx_bytes += *len;
        global_net_rx_packets++;
    }
    
    (void)iface;
    (void)data;
    (void)len;
    /* Would receive via network driver */
    return 0;
}

void socket_destroy(socket_t* sock) {
    VALIDATE_PTR_VOID(sock);
    
    spinlock_lock(&net_lock);
    
    /* Remove from socket list */
    if (socket_list == sock) {
        socket_list = (socket_t*)sock->private_data;
    } else {
        socket_t* current = socket_list;
        while (current && current->private_data != (void*)sock) {
            current = (socket_t*)current->private_data;
        }
        if (current) {
            current->private_data = sock->private_data;
        }
    }
    
    spinlock_unlock(&net_lock);
    
    kfree(sock);
    DEBUG_INFO("Socket destroyed");
}
