#ifndef ICMP_H
#define ICMP_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

/* Initialize ICMP */
void icmp_init(void);

/* Send ICMP echo request */
int icmp_send_echo(ip_addr_t dst, u16 id, u16 seq, const void* data, size_t len);

/* Receive ICMP packet from IP layer */
int icmp_recv_packet(sk_buff_t* skb);

#endif /* ICMP_H */
