#ifndef ROUTE_H
#define ROUTE_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

/* Route entry */
typedef struct route_entry route_entry_t;

/* Initialize routing */
void route_init(void);

/* Add/delete route */
int route_add(ip_addr_t dst, ip_addr_t netmask, ip_addr_t gateway, 
              netif_t* iface, u32 metric);
int route_del(ip_addr_t dst, ip_addr_t netmask);

/* Lookup route */
route_entry_t* route_lookup(ip_addr_t dst);

/* Forward packet */
int route_forward(sk_buff_t* skb, ip_addr_t dst);

#endif /* ROUTE_H */
