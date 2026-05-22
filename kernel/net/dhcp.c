#include "dhcp.h"
#include <stddef.h>
#include "udp.h"
#include "ip.h"
#include "net_socket.h"
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
static u32 dhcp_xid = 0x12345678;
static ip_addr_t dhcp_offered_ip;
static ip_addr_t dhcp_server_ip;
static bool dhcp_have_offer = false;

static u8 dhcp_option_type(const dhcp_packet_t* pkt, size_t pkt_len, u8 want) {
    if (pkt_len < offsetof(dhcp_packet_t, options)) {
        return 0;
    }
    const u8* opt = pkt->options;
    size_t left = pkt_len - offsetof(dhcp_packet_t, options);
    for (size_t i = 0; i + 1 < left;) {
        u8 t = opt[i++];
        if (t == 255) {
            break;
        }
        if (t == 0) {
            continue;
        }
        if (i >= left) {
            break;
        }
        u8 len = opt[i++];
        if (i + len > left) {
            break;
        }
        if (t == want && len >= 1) {
            return opt[i];
        }
        i += len;
    }
    return 0;
}

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
    pkt->xid = dhcp_xid;
    memcpy(pkt->chaddr, iface->mac, 6);
    pkt->magic = htonl(DHCP_MAGIC);

    u8* opt = pkt->options;
    *opt++ = 53;
    *opt++ = 1;
    *opt++ = 1;
    *opt++ = 255;

    socket_t* sock = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock || !sock->ops || !sock->ops->bind) {
        kfree(pkt);
        return -1;
    }

    sockaddr_t bind_addr;
    bind_addr.sa_family = AF_INET;
    u16 cport = htons(DHCP_CLIENT_PORT);
    memcpy(bind_addr.sa_data, &cport, 2);
    sock->ops->bind(sock, &bind_addr);

    ip_addr_t bcast = {{255, 255, 255, 255}};
    ssize_t ret = udp_send(sock, pkt, (size_t)(opt - (u8*)pkt), bcast, DHCP_SERVER_PORT);
    socket_destroy(sock);
    kfree(pkt);
    return ret > 0 ? 0 : -1;
}

static int dhcp_send_request(netif_t* iface) {
    if (!dhcp_have_offer) {
        return -1;
    }

    dhcp_packet_t* pkt = (dhcp_packet_t*)kzalloc(sizeof(dhcp_packet_t));
    if (!pkt) {
        return -1;
    }

    pkt->op = 1;
    pkt->htype = 1;
    pkt->hlen = 6;
    pkt->xid = dhcp_xid;
    memcpy(pkt->chaddr, iface->mac, 6);
    pkt->magic = htonl(DHCP_MAGIC);
    pkt->ciaddr = dhcp_offered_ip;

    u8* opt = pkt->options;
    *opt++ = 53;
    *opt++ = 1;
    *opt++ = 3;
    *opt++ = 50;
    *opt++ = 4;
    memcpy(opt, dhcp_offered_ip.addr, 4);
    opt += 4;
    *opt++ = 54;
    *opt++ = 4;
    memcpy(opt, dhcp_server_ip.addr, 4);
    opt += 4;
    *opt++ = 255;

    socket_t* sock = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock || !sock->ops || !sock->ops->bind) {
        kfree(pkt);
        return -1;
    }

    sockaddr_t bind_addr;
    bind_addr.sa_family = AF_INET;
    u16 cport = htons(DHCP_CLIENT_PORT);
    memcpy(bind_addr.sa_data, &cport, 2);
    sock->ops->bind(sock, &bind_addr);

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
    dhcp_have_offer = false;
    printk("[DHCP] DORA on %s...\n", iface->name);

    for (u32 round = 0; round < 24; round++) {
        dhcp_send_discover(iface);
        for (u32 j = 0; j < 80; j++) {
            net_poll();
        }
        if (dhcp_have_offer && !dhcp_complete) {
            dhcp_send_request(iface);
            for (u32 j = 0; j < 120; j++) {
                net_poll();
            }
        }
        if (dhcp_complete) {
            char buf[16];
            ip_addr_format(&iface->ip, buf, sizeof(buf));
            printk("[DHCP] ACK lease: %s gw ",
                   buf);
            ip_addr_format(&iface->gateway, buf, sizeof(buf));
            printk("%s\n", buf);
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

    u8 msg = dhcp_option_type(pkt, skb->len, 53);

    if (msg == 2) {
        if (pkt->yiaddr.addr[0] || pkt->yiaddr.addr[1] ||
            pkt->yiaddr.addr[2] || pkt->yiaddr.addr[3]) {
            dhcp_offered_ip = pkt->yiaddr;
            dhcp_server_ip = pkt->siaddr;
            if (!dhcp_server_ip.addr[0]) {
                dhcp_server_ip = pkt->giaddr;
            }
            dhcp_have_offer = true;
            printk("[DHCP] OFFER %u.%u.%u.%u\n",
                   dhcp_offered_ip.addr[0], dhcp_offered_ip.addr[1],
                   dhcp_offered_ip.addr[2], dhcp_offered_ip.addr[3]);
        }
        return 0;
    }

    if (msg == 5) {
        iface->ip = dhcp_have_offer ? dhcp_offered_ip : pkt->yiaddr;
        if (pkt->siaddr.addr[0]) {
            iface->gateway = pkt->siaddr;
        } else if (dhcp_server_ip.addr[0]) {
            iface->gateway = dhcp_server_ip;
        }
        dhcp_complete = true;
        return 0;
    }

    return 0;
}
