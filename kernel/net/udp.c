#include "net.h"
#include "ip.h"
#include "udp.h"
#include "dhcp.h"
#include "dns.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define UDP_HEADER_LEN 8
#define MAX_UDP_SOCKETS 1024
#define UDP_RECV_QUEUE_SIZE 64

/* UDP receive queue entry */
typedef struct udp_recv_entry {
    void* data;
    size_t len;
    ip_addr_t src_addr;
    u16 src_port;
    struct udp_recv_entry* next;
} udp_recv_entry_t;

/* UDP socket */
typedef struct udp_socket {
    ip_addr_t local_addr;
    u16 local_port;
    socket_t* sock;
    udp_recv_entry_t* recv_queue;
    u32 recv_queue_size;
    u32 max_recv_queue_size;
    struct udp_socket* next;
} udp_socket_t;

static udp_socket_t* udp_sockets = NULL;
static spinlock_t udp_lock = SPINLOCK_INIT;
static u16 udp_port_counter = 1024;
static u32 udp_socket_count = 0;

/**
 * @brief Initialize UDP protocol layer
 * 
 * Initializes the UDP protocol subsystem, including:
 * - Socket list
 * - Port counter
 * - Registration with IP layer
 * 
 * @note Must be called before any UDP operations
 */
void udp_init(void) {
    udp_sockets = NULL;
    udp_port_counter = 1024;
    
    /* Register UDP with IP layer */
    ip_register_protocol(IPPROTO_UDP, udp_recv_packet);
    
    printk("[UDP] UDP protocol initialized\n");
}

/**
 * @brief Parse sockaddr structure to extract IP and port
 * @param addr Socket address structure
 * @param ip Output IP address
 * @param port Output port number
 * @return 0 on success, -1 on error
 */
static int parse_sockaddr(const sockaddr_t* addr, ip_addr_t* ip, u16* port) {
    if (!addr || !ip || !port) {
        return -1;
    }
    
    if (addr->sa_family != 2) { /* AF_INET = 2 */
        return -1;
    }
    
    *port = ntohs(*(u16*)&addr->sa_data[0]);
    memcpy(ip, &addr->sa_data[2], 4);
    
    return 0;
}

static int udp_bind(socket_t* sock, const sockaddr_t* addr) {
    if (!sock || !addr) {
        return -1;
    }
    
    ip_addr_t bind_ip = {0};
    u16 port = 0;
    
    if (parse_sockaddr(addr, &bind_ip, &port) < 0) {
        return -1;
    }
    
    spinlock_lock(&udp_lock);
    
    /* Check socket limit */
    if (udp_socket_count >= MAX_UDP_SOCKETS) {
        spinlock_unlock(&udp_lock);
        return -1;
    }
    
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
    udp_sock->local_addr = bind_ip;
    udp_sock->local_port = port;
    udp_sock->recv_queue = NULL;
    udp_sock->recv_queue_size = 0;
    udp_sock->max_recv_queue_size = UDP_RECV_QUEUE_SIZE;
    udp_sock->next = udp_sockets;
    udp_sockets = udp_sock;
    udp_socket_count++;
    
    sock->private_data = udp_sock;
    sock->local_addr = bind_ip;
    sock->local_port = port;
    
    spinlock_unlock(&udp_lock);
    
    return 0;
}

ssize_t udp_send(socket_t* sock, const void* buf, size_t len, 
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

ssize_t udp_recv(socket_t* sock, void* buf, size_t len,
                        ip_addr_t* src_addr, u16* src_port) {
    if (!sock || !buf || len == 0) {
        return -1;
    }
    
    udp_socket_t* udp_sock = (udp_socket_t*)sock->private_data;
    if (!udp_sock) {
        return -1;
    }
    
    spinlock_lock(&udp_lock);
    
    /* Check if receive queue is empty */
    if (!udp_sock->recv_queue || udp_sock->recv_queue_size == 0) {
        spinlock_unlock(&udp_lock);
        return 0; /* No data available */
    }
    
    /* Get first entry from receive queue */
    udp_recv_entry_t* entry = udp_sock->recv_queue;
    udp_sock->recv_queue = entry->next;
    udp_sock->recv_queue_size--;
    
    spinlock_unlock(&udp_lock);
    
    /* Copy data */
    size_t to_copy = len;
    if (to_copy > entry->len) {
        to_copy = entry->len;
    }
    memcpy(buf, entry->data, to_copy);
    
    /* Fill source address if provided */
    if (src_addr) {
        *src_addr = entry->src_addr;
    }
    if (src_port) {
        *src_port = entry->src_port;
    }
    
    /* Free entry */
    kfree(entry->data);
    kfree(entry);
    
    return to_copy;
}

int udp_recv_packet(sk_buff_t* skb) {
    if (!skb || skb->len < UDP_HEADER_LEN) {
        return -1;
    }
    
    udp_header_t* udph = (udp_header_t*)skb->data;
    if (skb->ip_hdr && !tcp_udp_checksum_valid(skb->ip_hdr, skb->data, skb->len, IPPROTO_UDP)) {
        DEBUG_ERROR("Invalid UDP checksum");
        skb_free(skb);
        return -1;
    }
    u16 src_port = ntohs(udph->src_port);
    u16 dst_port = ntohs(udph->dst_port);
    
    ip_addr_t src_addr = {{0, 0, 0, 0}};
    ip_addr_t dst_addr = {{0, 0, 0, 0}};
    if (skb->ip_hdr) {
        src_addr = skb->ip_hdr->src;
        dst_addr = skb->ip_hdr->dst;
    }

    skb_pull(skb, UDP_HEADER_LEN);

    if (dst_port == 68) {
        dhcp_handle_packet(skb, dst_addr, dst_port);
        skb_free(skb);
        return 0;
    }
    if (src_port == 53) {
        dns_handle_reply(skb, src_port);
        skb_free(skb);
        return 0;
    }
    
    /* Find socket */
    spinlock_lock(&udp_lock);
    udp_socket_t* udp_sock = udp_sockets;
    while (udp_sock) {
        if (udp_sock->local_port == dst_port) {
            /* Check receive queue limit */
            if (udp_sock->recv_queue_size >= udp_sock->max_recv_queue_size) {
                spinlock_unlock(&udp_lock);
                skb_free(skb);
                return -1; /* Receive queue full */
            }
            
            /* Allocate receive queue entry */
            udp_recv_entry_t* entry = (udp_recv_entry_t*)kzalloc(sizeof(udp_recv_entry_t));
            if (entry) {
                entry->len = skb->len;
                entry->data = kzalloc(skb->len);
                if (entry->data) {
                    memcpy(entry->data, skb->data, skb->len);
                    entry->src_addr = src_addr;
                    entry->src_port = src_port;
                    entry->next = udp_sock->recv_queue;
                    udp_sock->recv_queue = entry;
                    udp_sock->recv_queue_size++;
                } else {
                    kfree(entry);
                }
            }
            
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
