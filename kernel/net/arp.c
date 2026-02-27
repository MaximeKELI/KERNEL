#include "net.h"
#include "arp.h"
#include "ip.h"
#include "ethernet.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define ARP_HEADER_LEN 28
#define ARP_TABLE_SIZE 256
#define ARP_HTYPE_ETHERNET 1
#define ARP_PROTO_IP 0x0800

/* ARP operation codes */
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

/* ARP header */
typedef struct __packed {
    u16 htype;      /* Hardware type */
    u16 ptype;      /* Protocol type */
    u8 hlen;        /* Hardware address length */
    u8 plen;        /* Protocol address length */
    u16 op;         /* Operation */
    u8 sha[6];      /* Sender hardware address */
    u8 spa[4];      /* Sender protocol address */
    u8 tha[6];      /* Target hardware address */
    u8 tpa[4];      /* Target protocol address */
} arp_header_t;

/* ARP table entry */
typedef struct arp_entry {
    ip_addr_t ip;
    u8 mac[6];
    u64 timestamp;
    bool valid;
    struct arp_entry* next;
} arp_entry_t;

static arp_entry_t* arp_table[ARP_TABLE_SIZE];
static spinlock_t arp_lock = SPINLOCK_INIT;

static u32 arp_hash(ip_addr_t ip) {
    return (ip.addr[0] ^ ip.addr[1] ^ ip.addr[2] ^ ip.addr[3]) % ARP_TABLE_SIZE;
}

void arp_init(void) {
    memset(arp_table, 0, sizeof(arp_table));
    printk("[ARP] Address Resolution Protocol initialized\n");
}

int arp_send_request(ip_addr_t target_ip, netif_t* iface) {
    if (!iface) {
        return -1;
    }
    
    /* Allocate socket buffer */
    sk_buff_t* skb = skb_alloc(ARP_HEADER_LEN);
    if (!skb) {
        return -1;
    }
    
    /* Build ARP header */
    arp_header_t* arph = (arp_header_t*)skb_put(skb, ARP_HEADER_LEN);
    arph->htype = htons(ARP_HTYPE_ETHERNET);
    arph->ptype = htons(ARP_PROTO_IP);
    arph->hlen = 6;
    arph->plen = 4;
    arph->op = htons(ARP_OP_REQUEST);
    
    memcpy(arph->sha, iface->mac, 6);
    memcpy(arph->spa, &iface->ip, 4);
    memset(arph->tha, 0, 6);
    memcpy(arph->tpa, &target_ip, 4);
    
    /* Send via Ethernet */
    ethernet_device_t* eth_dev = ethernet_find_device(iface->name);
    if (eth_dev) {
        ethernet_send_packet(eth_dev, skb->data, skb->len);
    }
    
    skb_free(skb);
    
    return 0;
}

int arp_recv_packet(sk_buff_t* skb, netif_t* iface) {
    if (!skb || skb->len < ARP_HEADER_LEN || !iface) {
        return -1;
    }
    
    arp_header_t* arph = (arp_header_t*)skb->data;
    
    if (ntohs(arph->htype) != ARP_HTYPE_ETHERNET ||
        ntohs(arph->ptype) != ARP_PROTO_IP) {
        skb_free(skb);
        return -1;
    }
    
    ip_addr_t spa, tpa;
    memcpy(&spa, arph->spa, 4);
    memcpy(&tpa, arph->tpa, 4);
    
    if (arph->op == htons(ARP_OP_REQUEST)) {
        /* Check if request is for us */
        if (memcmp(&tpa, &iface->ip, sizeof(ip_addr_t)) == 0) {
            /* Send ARP reply */
            arph->op = htons(ARP_OP_REPLY);
            memcpy(arph->tha, arph->sha, 6);
            memcpy(arph->tpa, arph->spa, 4);
            memcpy(arph->sha, iface->mac, 6);
            memcpy(arph->spa, &iface->ip, 4);
            
            ethernet_device_t* eth_dev = ethernet_find_device(iface->name);
            if (eth_dev) {
                ethernet_send_packet(eth_dev, skb->data, skb->len);
            }
        }
    } else if (arph->op == htons(ARP_OP_REPLY)) {
        /* Update ARP table */
        u32 hash = arp_hash(spa);
        
        spinlock_lock(&arp_lock);
        
        arp_entry_t* entry = arp_table[hash];
        while (entry) {
            if (memcmp(&entry->ip, &spa, sizeof(ip_addr_t)) == 0) {
                memcpy(entry->mac, arph->sha, 6);
                entry->valid = true;
                entry->timestamp = 0; /* TODO: Use actual timestamp */
                break;
            }
            entry = entry->next;
        }
        
        if (!entry) {
            /* Create new entry */
            entry = (arp_entry_t*)kzalloc(sizeof(arp_entry_t));
            if (entry) {
                entry->ip = spa;
                memcpy(entry->mac, arph->sha, 6);
                entry->valid = true;
                entry->timestamp = 0;
                entry->next = arp_table[hash];
                arp_table[hash] = entry;
            }
        }
        
        spinlock_unlock(&arp_lock);
    }
    
    skb_free(skb);
    return 0;
}

int arp_lookup(ip_addr_t ip, u8* mac) {
    if (!mac) {
        return -1;
    }
    
    u32 hash = arp_hash(ip);
    
    spinlock_lock(&arp_lock);
    
    arp_entry_t* entry = arp_table[hash];
    while (entry) {
        if (memcmp(&entry->ip, &ip, sizeof(ip_addr_t)) == 0 && entry->valid) {
            memcpy(mac, entry->mac, 6);
            spinlock_unlock(&arp_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&arp_lock);
    
    return -1; /* Not found */
}
