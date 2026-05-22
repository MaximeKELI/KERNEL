#include "dns.h"
#include "udp.h"
#include "ip.h"
#include "net.h"
#include "net_addr.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

#define DNS_PORT 53

static ip_addr_t dns_server = {{8, 8, 8, 8}};
static ip_addr_t dns_last_result;
static bool dns_have_result = false;

void dns_init(void) {
    dns_have_result = false;
    printk("[DNS] Resolver ready (server 8.8.8.8)\n");
}

void dns_set_server(ip_addr_t server) {
    dns_server = server;
}

static int dns_encode_query(const char* host, u8* buf, size_t buflen) {
    if (!host || buflen < 18) {
        return -1;
    }
    memset(buf, 0, buflen);
    u16 id = 0xBEEF;
    buf[0] = (u8)(id >> 8);
    buf[1] = (u8)(id & 0xFF);
    buf[2] = 0x01;
    buf[5] = 1;
    u8* p = buf + 12;
    const char* label = host;
    while (*label) {
        const char* dot = label;
        while (*dot && *dot != '.') {
            dot++;
        }
        size_t len = (size_t)(dot - label);
        if (len > 63 || p - buf > 200) {
            return -1;
        }
        *p++ = (u8)len;
        memcpy(p, label, len);
        p += len;
        if (!*dot) {
            break;
        }
        label = dot + 1;
    }
    *p++ = 0;
    *p++ = 0;
    *p++ = 1;
    *p++ = 0;
    *p++ = 1;
    return (int)(p - buf);
}

int dns_resolve_a(const char* hostname, ip_addr_t* out) {
    if (!hostname || !out) {
        return -1;
    }

    u8 query[256];
    int qlen = dns_encode_query(hostname, query, sizeof(query));
    if (qlen < 0) {
        return -1;
    }

    dns_have_result = false;
    socket_t* sock = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock) {
        return -1;
    }

    udp_send(sock, query, (size_t)qlen, dns_server, DNS_PORT);
    socket_destroy(sock);

    for (u32 i = 0; i < 128; i++) {
        net_poll();
        if (dns_have_result) {
            *out = dns_last_result;
            return 0;
        }
    }
    return -1;
}

int dns_handle_reply(sk_buff_t* skb, u16 src_port) {
    if (!skb || src_port != DNS_PORT || skb->len < 16) {
        return -1;
    }

    u8* p = skb->data;
    u16 ancount = (u16)((p[6] << 8) | p[7]);
    if (ancount == 0) {
        return -1;
    }

    p += 12;
    while (*p != 0) {
        if ((*p & 0xC0) == 0xC0) {
            p += 2;
            break;
        }
        p += 1 + *p;
    }
    if (*p == 0) {
        p++;
    }
    p += 4;
    if ((size_t)(p - skb->data) + 12 > skb->len) {
        return -1;
    }
    p += 2;
    u16 rdlen = (u16)((p[0] << 8) | p[1]);
    p += 2;
    if (rdlen == 4) {
        memcpy(dns_last_result.addr, p, 4);
        dns_have_result = true;
        return 0;
    }
    return -1;
}
