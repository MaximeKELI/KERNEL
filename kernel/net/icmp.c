#include "net.h"
#include "ip.h"
#include "icmp.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "types.h"
#include "string.h"

#define ICMP_HEADER_LEN 8

/* ICMP types */
#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8
#define ICMP_DEST_UNREACH 3
#define ICMP_TIME_EXCEEDED 11

/* ICMP header */
typedef struct __packed {
    u8 type;
    u8 code;
    u16 checksum;
    union {
        struct {
            u16 id;
            u16 seq;
        } echo;
        u32 unused;
    } un;
} icmp_header_t;

void icmp_init(void) {
    /* Register ICMP with IP layer */
    ip_register_protocol(IPPROTO_ICMP, icmp_recv_packet);
    
    printk("[ICMP] ICMP protocol initialized\n");
}

int icmp_send_echo(ip_addr_t dst, u16 id, u16 seq, const void* data, size_t len) {
    if (len > 1472) { /* Max ICMP payload */
        return -1;
    }
    
    /* Allocate socket buffer */
    sk_buff_t* skb = skb_alloc(len + ICMP_HEADER_LEN);
    if (!skb) {
        return -1;
    }
    
    skb_reserve(skb, ICMP_HEADER_LEN);
    if (data && len > 0) {
        memcpy(skb_put(skb, len), data, len);
    }
    
    /* Build ICMP header */
    icmp_header_t* icmph = (icmp_header_t*)skb_push(skb, ICMP_HEADER_LEN);
    icmph->type = ICMP_ECHO_REQUEST;
    icmph->code = 0;
    icmph->un.echo.id = htons(id);
    icmph->un.echo.seq = htons(seq);
    icmph->checksum = 0;
    
    /* Calculate checksum */
    icmph->checksum = ip_checksum(icmph, skb->len);
    
    /* Send via IP layer */
    int ret = ip_send_packet(dst, IPPROTO_ICMP, skb->data, skb->len);
    
    skb_free(skb);
    
    return ret;
}

int icmp_recv_packet(sk_buff_t* skb) {
    if (!skb || skb->len < ICMP_HEADER_LEN) {
        return -1;
    }
    
    icmp_header_t* icmph = (icmp_header_t*)skb->data;
    
    /* Verify checksum */
    u16 checksum = icmph->checksum;
    icmph->checksum = 0;
    if (ip_checksum(icmph, skb->len) != checksum) {
        DEBUG_ERROR("Invalid ICMP checksum");
        skb_free(skb);
        return -1;
    }
    
    /* Get source IP from IP header */
    ip_addr_t src = {0};
    if (skb->ip_hdr) {
        src = skb->ip_hdr->src;
    }
    
    switch (icmph->type) {
        case ICMP_ECHO_REQUEST:
            /* Send echo reply */
            icmph->type = ICMP_ECHO_REPLY;
            icmph->checksum = 0;
            icmph->checksum = ip_checksum(icmph, skb->len);
            
            ip_send_packet(src, IPPROTO_ICMP, skb->data, skb->len);
            break;
            
        case ICMP_ECHO_REPLY:
            /* Handle echo reply - typically used by ping */
            /* In a full implementation, this would wake up waiting processes */
            DEBUG_INFO("ICMP echo reply received from %u.%u.%u.%u",
                      src.addr[0], src.addr[1], src.addr[2], src.addr[3]);
            break;
            
        case ICMP_DEST_UNREACH:
            /* Handle destination unreachable */
            DEBUG_INFO("ICMP destination unreachable: code=%u", icmph->code);
            /* In a full implementation, this would notify the sending socket */
            break;
            
        case ICMP_TIME_EXCEEDED:
            /* Handle time exceeded */
            DEBUG_INFO("ICMP time exceeded: code=%u", icmph->code);
            /* In a full implementation, this would notify routing layer */
            break;
            
        default:
            DEBUG_INFO("ICMP message type %u not handled", icmph->type);
            break;
    }
    
    skb_free(skb);
    return 0;
}
