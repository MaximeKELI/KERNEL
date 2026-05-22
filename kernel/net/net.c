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
#include "eth.h"
#include "net_addr.h"
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

u64 global_net_tx_bytes = 0;
u64 global_net_rx_bytes = 0;
u64 global_net_tx_packets = 0;
u64 global_net_rx_packets = 0;

netif_t* netif_list = NULL;
static netif_t default_netif;
static socket_t* socket_list = NULL;
static spinlock_t net_lock = SPINLOCK_INIT;

netif_t* net_default_if(void) {
    return netif_list;
}

static void net_setup_default_interface(void) {
    ethernet_device_t* eth = ethernet_find_device("eth0");
    if (!eth) {
        return;
    }

    memset(&default_netif, 0, sizeof(default_netif));
    strncpy(default_netif.name, "eth0", sizeof(default_netif.name) - 1);
    ip_addr_parse("10.0.2.15", &default_netif.ip);
    ip_addr_parse("255.255.255.0", &default_netif.netmask);
    ip_addr_parse("10.0.2.2", &default_netif.gateway);
    ethernet_get_mac(eth, default_netif.mac);
    default_netif.up = true;

    ethernet_up(eth);
    netif_register(&default_netif);

    ip_addr_t net_dst = {{10, 0, 2, 0}};
    ip_addr_t zero = {{0, 0, 0, 0}};
    route_add(net_dst, default_netif.netmask, zero, &default_netif, 1);

    route_add(zero, zero, default_netif.gateway, &default_netif, 0);

    u8 gw_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    arp_add_entry(default_netif.gateway, gw_mac);

    char ipbuf[16];
    ip_addr_format(&default_netif.ip, ipbuf, sizeof(ipbuf));
    printk("[Net] Interface %s up, IP %s\n", default_netif.name, ipbuf);
}

void net_init(void) {
    eth_loopback_init();
    ethernet_init();

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

    kernel_bypass_init();
    hw_offload_init();
    zero_copy_init();
    adaptive_routing_init();
    network_slicing_init();

    net_setup_default_interface();

    DEBUG_INFO("Networking stack initialized");
    printk("[Net] Stack ready (L2 loopback + L3/L4)\n");
}

socket_t* socket_create(int domain, int type, int protocol) {
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

    if (type == SOCK_STREAM) {
        sock->ops = &tcp_ops;
    } else if (type == SOCK_DGRAM) {
        sock->ops = &udp_ops;
    }

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

int net_process_packet(netif_t* iface, void* data, size_t len) {
    if (!iface || !data || len < ETH_HDR_LEN) {
        return -1;
    }

    eth_header_t* eth = (eth_header_t*)data;
    u16 proto = ntohs(eth->proto);
    size_t payload_len = len - ETH_HDR_LEN;
    void* payload = (u8*)data + ETH_HDR_LEN;

    sk_buff_t* skb = skb_alloc(payload_len);
    if (!skb) {
        net_stats_update_rx_error();
        return -1;
    }
    memcpy(skb_put(skb, payload_len), payload, payload_len);
    skb->protocol = proto;

    u32 verdict = netfilter_hook(NF_INET_PRE_ROUTING, skb, iface, NULL);
    if (verdict == NF_DROP) {
        skb_free(skb);
        return -1;
    }

    if (proto == ETH_P_ARP) {
        return arp_recv_packet(skb, iface);
    }
    if (proto == ETH_P_IP) {
        return ip_recv_packet(skb);
    }

    skb_free(skb);
    return -1;
}

void net_poll(void) {
    netif_t* iface = netif_list;
    while (iface) {
        if (iface->up) {
            while (eth_poll(iface) > 0) {
                /* drain loopback queue */
            }
        }
        iface = iface->next;
    }
}

int net_send_packet(netif_t* iface, const void* data, size_t len) {
    VALIDATE_PTR(iface);
    VALIDATE_PTR(data);
    VALIDATE_RANGE(len, 0, 65535);

    if (!iface->up || len < IP_HEADER_LEN) {
        return -1;
    }

    ip_header_t* iph = (ip_header_t*)data;
    u8 mac[6];
    if (arp_resolve(iface, iph->dst, mac) < 0) {
        return -1;
    }
    return eth_transmit(iface, ETH_P_IP, mac, data, len);
}

int net_recv_packet(netif_t* iface, void* data, size_t* len) {
    VALIDATE_PTR(iface);
    VALIDATE_PTR(data);
    VALIDATE_PTR(len);

    u8 buf[ETH_FRAME_MAX];
    int n = eth_loop_dequeue(iface->name, buf, sizeof(buf));
    if (n <= 0) {
        return -1;
    }
    if ((size_t)n > *len) {
        return -1;
    }
    memcpy(data, buf, (size_t)n);
    *len = (size_t)n;
    net_process_packet(iface, buf, (size_t)n);
    return 0;
}

void socket_destroy(socket_t* sock) {
    VALIDATE_PTR_VOID(sock);

    spinlock_lock(&net_lock);

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
