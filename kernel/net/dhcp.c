#include "dhcp.h"
#include "udp.h"
#include "ip.h"
#include "net_addr.h"
#include "net.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAGIC 0x63825363

typedef struct __packed {
    u8 op;
    u8 htype;
    u8 hlen;
    u8 hops;
    u32 xid;
    u16 secs;
    u16 flags;
    ip_addr_t ciaddr;
    ip_addr_t yiaddr;
    ip_addr_t siaddr;
    ip_addr_t giaddr;
    u8 chaddr[16];
    u8 sname[64];
    u8 file[128];
    u32 magic;
    u8 options[312];
} dhcp_packet_t;

static bool dhcp_complete = false;

void dhcp_init(void) {
    dhcp_complete = false;
}

bool net_dhcp_done(void) {
    return dhcp_complete;
}

static int dhcp_send_discover(netif_t* iface) {
    dhcp_packet_t* pkt = (dhcp_packet_t*)kzalloc(sizeof(dhcp_packet_t));
    if (!pkt) {
        return -1;
    }

    pkt->op = 1;
    pkt->htype = 1;
    pkt->hlen = 6;
    pkt->xid = 0x12345678;
    memcpy(pkt->chaddr, iface->mac, 6);
    pkt->magic = htonl(DHCP_MAGIC);

    u8* opt = pkt->options;
    *opt++ = 53;
    *opt++ = 1;
    *opt++ = 1;
    *opt++ = 255;

    socket_t* sock = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock) {
        kfree(pkt);
        return -1;
    }

    ip_addr_t bcast = {{255, 255, 255, 255}};
    ssize_t ret = udp_send(sock, pkt, (size_t)(opt - (u8*)pkt), bcast, DHCP_SERVER_PORT);
    socket_destroy(sock);
    kfree(pkt);
    return ret > 0 ? 0 : -1;
}

int net_dhcp_acquire(netif_t* iface) {
    if (!iface) {
        return -1;
    }
    dhcp_complete = false;
    printk("[DHCP] Discover on %s...\n", iface->name);

    for (u32 i = 0; i < 32; i++) {
        dhcp_send_discover(iface);
        for (u32 j = 0; j < 64; j++) {
            net_poll();
        }
        if (dhcp_complete) {
            char buf[16];
            ip_addr_format(&iface->ip, buf, sizeof(buf));
            printk("[DHCP] Lease: %s\n", buf);
            return 0;
        }
    }
    printk("[DHCP] Timeout — keeping static IP\n");
    return -1;
}

int dhcp_handle_packet(sk_buff_t* skb, ip_addr_t dst, u16 dst_port) {
    (void)dst;
    if (!skb || dst_port != DHCP_CLIENT_PORT || skb->len < 240) {
        return -1;
    }

    dhcp_packet_t* pkt = (dhcp_packet_t*)skb->data;
    if (ntohl(pkt->magic) != DHCP_MAGIC) {
        return -1;
    }

    extern netif_t* netif_list;
    netif_t* iface = netif_list;
    if (!iface) {
        return -1;
    }

    if (pkt->yiaddr.addr[0] || pkt->yiaddr.addr[1] || pkt->yiaddr.addr[2] || pkt->yiaddr.addr[3]) {
        iface->ip = pkt->yiaddr;
        if (pkt->siaddr.addr[0]) {
            iface->gateway = pkt->siaddr;
        }
        dhcp_complete = true;
    }
    return 0;
}
