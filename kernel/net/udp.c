#include "net.h"
#include "ip.h"
#include "udp.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define UDP_HEADER_LEN 8
#define MAX_UDP_SOCKETS 256

/* UDP socket */
typedef struct udp_socket {
    ip_addr_t local_addr;
    u16 local_port;
    socket_t* sock;
    struct udp_socket* next;
} udp_socket_t;

static udp_socket_t* udp_sockets = NULL;
static spinlock_t udp_lock = SPINLOCK_INIT;
static u16 udp_port_counter = 1024;

void udp_init(void) {
    udp_sockets = NULL;
    udp_port_counter = 1024;
    
    /* Register UDP with IP layer */
    ip_register_protocol(IPPROTO_UDP, udp_recv_packet);
    
    printk("[UDP] UDP protocol initialized\n");
}

static int udp_bind(socket_t* sock, const sockaddr_t* addr) {
    if (!sock || !addr) {
        return -1;
    }
    
    /* TODO: Parse sockaddr to get IP and port */
    u16 port = 0;
    
    spinlock_lock(&udp_lock);
    
    /* Check if port is already in use */
    udp_socket_t* s = udp_sockets;
    while (s) {
        if (s->local_port == port) {
            spinlock_unlock(&udp_lock);
            return -1; /* Port in use */
        }
        s = s->next;
    }
    
    /* Allocate UDP socket */
    udp_socket_t* udp_sock = (udp_socket_t*)kzalloc(sizeof(udp_socket_t));
    if (!udp_sock) {
        spinlock_unlock(&udp_lock);
        return -1;
    }
    
    udp_sock->sock = sock;
    udp_sock->local_port = port;
    udp_sock->next = udp_sockets;
    udp_sockets = udp_sock;
    
    spinlock_unlock(&udp_lock);
    
    return 0;
}

static ssize_t udp_send(socket_t* sock, const void* buf, size_t len, 
                        ip_addr_t dst_addr, u16 dst_port) {
    if (!sock || !buf || len == 0) {
        return -1;
    }
    
    /* Allocate socket buffer */
    sk_buff_t* skb = skb_alloc(len + UDP_HEADER_LEN);
    if (!skb) {
        return -1;
    }
    
    skb_reserve(skb, UDP_HEADER_LEN);
    memcpy(skb_put(skb, len), buf, len);
    
    /* Build UDP header */
    udp_header_t* udph = (udp_header_t*)skb_push(skb, UDP_HEADER_LEN);
    
    /* Get local port from socket */
    udp_socket_t* udp_sock = (udp_socket_t*)sock->private_data;
    udph->src_port = htons(udp_sock ? udp_sock->local_port : udp_port_counter++);
    udph->dst_port = htons(dst_port);
    udph->length = htons(skb->len);
    udph->checksum = 0; /* Optional for UDP */
    
    /* Send via IP layer */
    int ret = ip_send_packet(dst_addr, IPPROTO_UDP, skb->data, skb->len);
    
    skb_free(skb);
    
    return ret == 0 ? len : -1;
}

static ssize_t udp_recv(socket_t* sock, void* buf, size_t len,
                        ip_addr_t* src_addr, u16* src_port) {
    if (!sock || !buf || len == 0) {
        return -1;
    }
    
    /* TODO: Receive from receive queue */
    (void)src_addr;
    (void)src_port;
    
    return 0;
}

int udp_recv_packet(sk_buff_t* skb) {
    if (!skb || skb->len < UDP_HEADER_LEN) {
        return -1;
    }
    
    udp_header_t* udph = (udp_header_t*)skb->data;
    
    /* Remove UDP header */
    skb_pull(skb, UDP_HEADER_LEN);
    
    /* Find socket */
    u16 dst_port = ntohs(udph->dst_port);
    
    spinlock_lock(&udp_lock);
    udp_socket_t* udp_sock = udp_sockets;
    while (udp_sock) {
        if (udp_sock->local_port == dst_port) {
            /* TODO: Deliver to socket receive queue */
            spinlock_unlock(&udp_lock);
            skb_free(skb);
            return 0;
        }
        udp_sock = udp_sock->next;
    }
    spinlock_unlock(&udp_lock);
    
    /* No socket found */
    skb_free(skb);
    return -1;
}

static int udp_close(socket_t* sock) {
    if (!sock) {
        return -1;
    }
    
    spinlock_lock(&udp_lock);
    
    udp_socket_t* prev = NULL;
    udp_socket_t* s = udp_sockets;
    while (s) {
        if (s->sock == sock) {
            if (prev) {
                prev->next = s->next;
            } else {
                udp_sockets = s->next;
            }
            kfree(s);
            break;
        }
        prev = s;
        s = s->next;
    }
    
    spinlock_unlock(&udp_lock);
    
    return 0;
}

socket_ops_t udp_ops = {
    .bind = udp_bind,
    .listen = NULL,
    .accept = NULL,
    .connect = NULL,
    .send = (ssize_t (*)(socket_t*, const void*, size_t))udp_send,
    .recv = (ssize_t (*)(socket_t*, void*, size_t))udp_recv,
    .close = udp_close
};
