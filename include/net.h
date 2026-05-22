#ifndef NET_H
#define NET_H

#include "types.h"

/* Network protocols */
#define ETH_P_IP  0x0800
#define ETH_P_ARP 0x0806

/* IP protocol */
#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17

/* Socket types */
#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

/* Socket address */
typedef struct sockaddr {
    u16 sa_family;
    char sa_data[14];
} sockaddr_t;

/* IP address */
typedef struct {
    u8 addr[4];
} ip_addr_t;

/* IP header (declared here to avoid circular includes with skbuff.h) */
typedef struct __packed {
    u8 version_ihl;
    u8 tos;
    u16 total_length;
    u16 id;
    u16 flags_fragment;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    ip_addr_t src;
    ip_addr_t dst;
} ip_header_t;

/* Socket structure */
typedef struct socket {
    u32 domain;
    u32 type;
    u32 protocol;
    ip_addr_t local_addr;
    u16 local_port;
    ip_addr_t remote_addr;
    u16 remote_port;
    void* private_data;
    struct socket_ops* ops;
} socket_t;

/* Socket operations */
typedef struct socket_ops {
    int (*bind)(socket_t* sock, const sockaddr_t* addr);
    int (*listen)(socket_t* sock, int backlog);
    socket_t* (*accept)(socket_t* sock, sockaddr_t* addr);
    int (*connect)(socket_t* sock, const sockaddr_t* addr);
    ssize_t (*send)(socket_t* sock, const void* buf, size_t len);
    ssize_t (*recv)(socket_t* sock, void* buf, size_t len);
    int (*close)(socket_t* sock);
} socket_ops_t;

/* Network interface */
typedef struct netif {
    char name[16];
    ip_addr_t ip;
    ip_addr_t netmask;
    ip_addr_t gateway;
    u8 mac[6];
    bool up;
    struct netif* next;
} netif_t;

/* Global interface list (defined in net.c) */
extern netif_t* netif_list;

/* Initialize networking */
void net_init(void);

/* Create socket */
socket_t* socket_create(int domain, int type, int protocol);
void socket_destroy(socket_t* sock);

/* Register network interface */
int netif_register(netif_t* iface);

/* Send packet */
int net_send_packet(netif_t* iface, const void* data, size_t len);

/* Receive packet */
int net_recv_packet(netif_t* iface, void* data, size_t* len);

/* Process received packet (full stack) */
int net_process_packet(netif_t* iface, void* data, size_t len);

/* Poll RX queues (loopback / driver) */
void net_poll(void);

/* Default interface */
netif_t* net_default_if(void);

#endif /* NET_H */
