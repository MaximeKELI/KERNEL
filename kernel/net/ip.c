#include "net.h"
#include "ip.h"
#include "skbuff.h"
#include "ethernet.h"
#include "route.h"
#include "rcu.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define IP_VERSION 4
#define IP_HEADER_LEN 20

static u16 ip_id_counter = 0;
static spinlock_t ip_lock = SPINLOCK_INIT;

/* IP protocol handlers */
typedef int (*ip_protocol_handler_t)(sk_buff_t* skb);
static ip_protocol_handler_t ip_protocols[256];
static spinlock_t ip_protocols_lock = SPINLOCK_INIT;

/**
 * @brief Initialize IP protocol layer
 * 
 * Initializes the IP protocol subsystem, including:
 * - IP ID counter
 * - Protocol handler table
 * 
 * @note Must be called before any IP operations
 */
void ip_init(void) {
    ip_id_counter = 0;
    memset(ip_protocols, 0, sizeof(ip_protocols));
    printk("[IP] IP protocol layer initialized\n");
}

/**
 * @brief Calculate IP checksum
 * @param data Pointer to data
 * @param len Length of data in bytes
 * @return 16-bit checksum value
 * 
 * Calculates the standard IP checksum (one's complement sum).
 */
u16 ip_checksum(const void* data, size_t len) {
    const u16* words = (const u16*)data;
    u32 sum = 0;
    
    for (size_t i = 0; i < len / 2; i++) {
        sum += words[i];
    }
    
    if (len % 2) {
        sum += ((u8*)data)[len - 1] << 8;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/**
 * @brief Send an IP packet
 * @param dst Destination IP address
 * @param protocol IP protocol number (TCP, UDP, ICMP, etc.)
 * @param data Packet payload data
 * @param len Length of payload
 * @return 0 on success, -1 on error
 * 
 * Builds an IP packet with header and sends it via the network interface.
 */
int ip_send_packet(ip_addr_t dst, u8 protocol, const void* data, size_t len) {
    if (!data || len == 0) {
        return -1;
    }
    
    /* Allocate socket buffer */
    sk_buff_t* skb = skb_alloc(len + IP_HEADER_LEN);
    if (!skb) {
        DEBUG_ERROR("Failed to allocate sk_buff for IP packet");
        return -1;
    }
    
    /* Reserve space for IP header */
    skb_reserve(skb, IP_HEADER_LEN);
    
    /* Copy data */
    memcpy(skb_put(skb, len), data, len);
    
    /* Build IP header */
    ip_header_t* iph = (ip_header_t*)skb_push(skb, IP_HEADER_LEN);
    iph->version_ihl = (IP_VERSION << 4) | (IP_HEADER_LEN / 4);
    iph->tos = 0;
    iph->total_length = htons(skb->len);
    
    spinlock_lock(&ip_lock);
    iph->id = htons(ip_id_counter++);
    spinlock_unlock(&ip_lock);
    
    iph->flags_fragment = htons(0x4000); /* Don't fragment */
    iph->ttl = 64;
    iph->protocol = protocol;
    iph->checksum = 0;
    
    /* Get source IP from interface */
    extern netif_t* netif_list;
    netif_t* iface = netif_list;
    if (iface && iface->up) {
        iph->src = iface->ip;
    } else {
        memset(&iph->src, 0, sizeof(ip_addr_t));
    }
    
    iph->dst = dst;
    
    /* Calculate checksum */
    iph->checksum = ip_checksum(iph, IP_HEADER_LEN);
    
    /* Send via network interface */
    if (iface) {
        ethernet_send_packet(ethernet_find_device(iface->name), skb->data, skb->len);
    }
    
    skb_free(skb);
    
    return 0;
}

int ip_recv_packet(sk_buff_t* skb) {
    if (!skb || skb->len < IP_HEADER_LEN) {
        return -1;
    }
    
    ip_header_t* iph = (ip_header_t*)skb->data;
    
    /* Store IP header pointer in skb */
    skb->ip_hdr = iph;
    
    /* Verify version */
    if ((iph->version_ihl >> 4) != IP_VERSION) {
        DEBUG_ERROR("Invalid IP version");
        skb_free(skb);
        return -1;
    }
    
    /* Verify checksum */
    u16 checksum = iph->checksum;
    iph->checksum = 0;
    if (ip_checksum(iph, IP_HEADER_LEN) != checksum) {
        DEBUG_ERROR("Invalid IP checksum");
        skb_free(skb);
        return -1;
    }
    iph->checksum = checksum; /* Restore checksum */
    
    /* Check if packet is for us */
    extern netif_t* netif_list;
    netif_t* iface = netif_list;
    bool for_us = false;
    
    if (iface) {
        if (memcmp(&iph->dst, &iface->ip, sizeof(ip_addr_t)) == 0) {
            for_us = true;
        }
    }
    
    if (!for_us) {
        /* Forward packet */
        extern int route_forward(sk_buff_t* skb, ip_addr_t dst);
        int ret = route_forward(skb, iph->dst);
        if (ret < 0) {
            skb_free(skb);
        }
        return ret;
    }
    
    /* Remove IP header */
    skb_pull(skb, IP_HEADER_LEN);
    
    /* Call protocol handler with RCU protection */
    ip_protocol_handler_t handler = NULL;
    rcu_read_lock();
    spinlock_lock(&ip_protocols_lock);
    handler = ip_protocols[iph->protocol];
    spinlock_unlock(&ip_protocols_lock);
    rcu_read_unlock();
    
    if (handler) {
        return handler(skb);
    }
    
    DEBUG_ERROR("No handler for IP protocol %u", iph->protocol);
    skb_free(skb);
    return -1;
}

void ip_register_protocol(u8 protocol, ip_protocol_handler_t handler) {
    if (protocol < 256) {
        spinlock_lock(&ip_protocols_lock);
        ip_protocols[protocol] = handler;
        spinlock_unlock(&ip_protocols_lock);
        synchronize_rcu(); /* Ensure all readers see the update */
        DEBUG_INFO("IP protocol %u registered", protocol);
    }
}

u16 htons(u16 hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

u32 htonl(u32 hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong >> 24) & 0xFF);
}

u16 ntohs(u16 netshort) {
    return htons(netshort);
}

u32 ntohl(u32 netlong) {
    return htonl(netlong);
}
