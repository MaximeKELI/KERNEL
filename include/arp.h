#ifndef ARP_H
#define ARP_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

/* Initialize ARP */
void arp_init(void);

/* Send ARP request */
int arp_send_request(ip_addr_t target_ip, netif_t* iface);

/* Receive ARP packet */
int arp_recv_packet(sk_buff_t* skb, netif_t* iface);

/* Lookup MAC address */
int arp_lookup(ip_addr_t ip, u8* mac);

#endif /* ARP_H */
