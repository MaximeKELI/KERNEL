#ifndef IP_H
#define IP_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

/* Initialize IP layer */
void ip_init(void);

/* Send IP packet */
int ip_send_packet(ip_addr_t dst, u8 protocol, const void* data, size_t len);

/* Receive IP packet */
int ip_recv_packet(sk_buff_t* skb);

/* Register IP protocol handler */
typedef int (*ip_protocol_handler_t)(sk_buff_t* skb);
void ip_register_protocol(u8 protocol, ip_protocol_handler_t handler);

/* Network byte order conversion */
u16 htons(u16 hostshort);
u32 htonl(u32 hostlong);
u16 ntohs(u16 netshort);
u32 ntohl(u32 netlong);

/* IP checksum */
u16 ip_checksum(const void* data, size_t len);

/* TCP/UDP pseudo-header checksum (data = transport hdr + payload) */
u16 tcp_udp_checksum(const ip_header_t* iph, const void* data, size_t len, u8 protocol);
bool tcp_udp_checksum_valid(const ip_header_t* iph, const void* data, size_t len, u8 protocol);

/* IP header length */
#define IP_HEADER_LEN 20

#endif /* IP_H */
