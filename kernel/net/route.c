#include "route.h"
#include "net.h"
#include "ip.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_ROUTES 256

/* Routing table entry */
typedef struct route_entry {
    ip_addr_t dst;
    ip_addr_t netmask;
    ip_addr_t gateway;
    netif_t* iface;
    u32 metric;
    u32 flags;
    struct route_entry* next;
} route_entry_t;

static route_entry_t* route_table = NULL;
static spinlock_t route_lock = SPINLOCK_INIT;

void route_init(void) {
    route_table = NULL;
    printk("[Route] Routing table initialized\n");
}

int route_add(ip_addr_t dst, ip_addr_t netmask, ip_addr_t gateway, 
                netif_t* iface, u32 metric) {
    if (!iface) {
        return -1;
    }
    
    route_entry_t* route = (route_entry_t*)kzalloc(sizeof(route_entry_t));
    if (!route) {
        return -1;
    }
    
    route->dst = dst;
    route->netmask = netmask;
    route->gateway = gateway;
    route->iface = iface;
    route->metric = metric;
    route->flags = 0;
    
    spinlock_lock(&route_lock);
    route->next = route_table;
    route_table = route;
    spinlock_unlock(&route_lock);
    
    DEBUG_INFO("Route added: %u.%u.%u.%u/%u.%u.%u.%u via %u.%u.%u.%u",
              dst.addr[0], dst.addr[1], dst.addr[2], dst.addr[3],
              netmask.addr[0], netmask.addr[1], netmask.addr[2], netmask.addr[3],
              gateway.addr[0], gateway.addr[1], gateway.addr[2], gateway.addr[3]);
    
    return 0;
}

int route_del(ip_addr_t dst, ip_addr_t netmask) {
    spinlock_lock(&route_lock);
    
    route_entry_t* prev = NULL;
    route_entry_t* route = route_table;
    
    while (route) {
        if (memcmp(&route->dst, &dst, sizeof(ip_addr_t)) == 0 &&
            memcmp(&route->netmask, &netmask, sizeof(ip_addr_t)) == 0) {
            if (prev) {
                prev->next = route->next;
            } else {
                route_table = route->next;
            }
            kfree(route);
            spinlock_unlock(&route_lock);
            return 0;
        }
        prev = route;
        route = route->next;
    }
    
    spinlock_unlock(&route_lock);
    return -1;
}

route_entry_t* route_lookup(ip_addr_t dst) {
    route_entry_t* best_route = NULL;
    u32 best_metric = UINT32_MAX;
    
    spinlock_lock(&route_lock);
    
    route_entry_t* route = route_table;
    while (route) {
        /* Check if destination matches */
        bool matches = true;
        for (int i = 0; i < 4; i++) {
            if ((dst.addr[i] & route->netmask.addr[i]) != 
                (route->dst.addr[i] & route->netmask.addr[i])) {
                matches = false;
                break;
            }
        }
        
        if (matches && route->metric < best_metric) {
            best_route = route;
            best_metric = route->metric;
        }
        
        route = route->next;
    }
    
    spinlock_unlock(&route_lock);
    
    return best_route;
}

int route_forward(sk_buff_t* skb, ip_addr_t dst) {
    route_entry_t* route = route_lookup(dst);
    if (!route) {
        DEBUG_ERROR("No route to destination");
        return -1;
    }
    
    /* Decrement TTL */
    ip_header_t* iph = (ip_header_t*)skb->data;
    if (iph->ttl <= 1) {
        DEBUG_ERROR("TTL expired");
        skb_free(skb);
        return -1;
    }
    iph->ttl--;
    
    /* Recalculate checksum */
    iph->checksum = 0;
    iph->checksum = ip_checksum(iph, IP_HEADER_LEN);
    
    /* Send via interface */
    if (route->gateway.addr[0] != 0 || route->gateway.addr[1] != 0 ||
        route->gateway.addr[2] != 0 || route->gateway.addr[3] != 0) {
        /* Send to gateway */
        return net_send_packet(route->iface, skb->data, skb->len);
    } else {
        /* Direct route */
        return net_send_packet(route->iface, skb->data, skb->len);
    }
}
