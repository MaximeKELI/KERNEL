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

/* Resolve MAC (lookup, gateway ARP, or broadcast) */
int arp_resolve(netif_t* iface, ip_addr_t ip, u8* mac);

/* Seed static ARP entry (e.g. gateway) */
int arp_add_entry(ip_addr_t ip, const u8* mac);

#endif /* ARP_H */
