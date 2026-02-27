#ifndef UDP_H
#define UDP_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

/* UDP header */
typedef struct __packed {
    u16 src_port;
    u16 dst_port;
    u16 length;
    u16 checksum;
} udp_header_t;

/* Initialize UDP */
void udp_init(void);

/* UDP socket operations */
ssize_t udp_send(socket_t* sock, const void* buf, size_t len, 
                 ip_addr_t dst_addr, u16 dst_port);
ssize_t udp_recv(socket_t* sock, void* buf, size_t len,
                 ip_addr_t* src_addr, u16* src_port);

/* Receive UDP packet from IP layer */
int udp_recv_packet(sk_buff_t* skb);

/* UDP socket operations */
extern socket_ops_t udp_ops;

#endif /* UDP_H */
