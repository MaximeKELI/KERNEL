#ifndef DHCP_H
#define DHCP_H

#include "types.h"
#include "net.h"

void dhcp_init(void);
int net_dhcp_acquire(netif_t* iface);
int dhcp_handle_packet(sk_buff_t* skb, ip_addr_t dst, u16 dst_port);
bool net_dhcp_done(void);

#endif /* DHCP_H */
