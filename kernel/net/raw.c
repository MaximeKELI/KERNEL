#include "net_raw.h"
#include "ip.h"
#include "skbuff.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"

#define MAX_RAW_SOCKETS 32

typedef struct raw_socket {
    u8 protocol;
    socket_t* sock;
    struct raw_socket* next;
} raw_socket_t;

static raw_socket_t* raw_sockets = NULL;

static int raw_bind(socket_t* sock, const sockaddr_t* addr) {
    (void)addr;
    if (!sock) {
        return -1;
    }
    return 0;
}

static ssize_t raw_send(socket_t* sock, const void* buf, size_t len) {
    if (!sock || !buf || len == 0) {
        return -1;
    }
    raw_socket_t* rs = (raw_socket_t*)sock->private_data;
    u8 proto = rs ? rs->protocol : (u8)sock->protocol;
    if (proto == 0 || proto == IPPROTO_RAW) {
        proto = IPPROTO_ICMP;
    }
    ip_addr_t dst = sock->remote_addr;
    if (dst.addr[0] == 0 && dst.addr[1] == 0 && dst.addr[2] == 0 && dst.addr[3] == 0) {
        extern netif_t* netif_list;
        if (netif_list) {
            dst = netif_list->ip;
        }
    }
    int ret = ip_send_packet(dst, proto, buf, len);
    return ret == 0 ? (ssize_t)len : -1;
}

static ssize_t raw_recv(socket_t* sock, void* buf, size_t len) {
    (void)sock;
    (void)buf;
    (void)len;
    return 0;
}

static int raw_close(socket_t* sock) {
    (void)sock;
    return 0;
}

socket_ops_t raw_ops = {
    .bind = raw_bind,
    .listen = NULL,
    .accept = NULL,
    .connect = NULL,
    .send = raw_send,
    .recv = raw_recv,
    .close = raw_close,
};

static raw_socket_t* raw_find_by_proto(u8 proto) {
    raw_socket_t* rs = raw_sockets;
    while (rs) {
        if (rs->protocol == proto || rs->protocol == 0) {
            return rs;
        }
        rs = rs->next;
    }
    return NULL;
}

int raw_attach_socket(socket_t* sock) {
    if (!sock) {
        return -1;
    }
    raw_socket_t* rs = (raw_socket_t*)kzalloc(sizeof(raw_socket_t));
    if (!rs) {
        return -1;
    }
    rs->protocol = (sock->protocol == 0) ? IPPROTO_RAW : (u8)sock->protocol;
    rs->sock = sock;
    rs->next = raw_sockets;
    raw_sockets = rs;
    sock->private_data = rs;
    return 0;
}

int raw_recv_packet(sk_buff_t* skb) {
    if (!skb || !skb->ip_hdr) {
        return -1;
    }
    raw_socket_t* rs = raw_find_by_proto(skb->ip_hdr->protocol);
    if (rs) {
        DEBUG_INFO("RAW socket received proto %u (%zu bytes)",
                   skb->ip_hdr->protocol, skb->len);
    }
    skb_free(skb);
    return 0;
}

void raw_init(void) {
    ip_register_protocol(IPPROTO_RAW, raw_recv_packet);
}
