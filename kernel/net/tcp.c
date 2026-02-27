#include "net.h"
#include "ip.h"
#include "tcp.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define TCP_HEADER_LEN 20
#define MAX_TCP_CONNECTIONS 1024
#define TCP_WINDOW_SIZE 65535
#define TCP_DEFAULT_TIMEOUT_MS 30000  /* 30 seconds */
#define TCP_ACCEPT_BACKLOG_MAX 128
#define TCP_RECV_BUFFER_SIZE (64 * 1024)  /* 64KB */

/* TCP connection states */
#define TCP_CLOSED      0
#define TCP_LISTEN      1
#define TCP_SYN_SENT    2
#define TCP_SYN_RCVD    3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT1   5
#define TCP_FIN_WAIT2   6
#define TCP_CLOSE_WAIT  7
#define TCP_CLOSING     8
#define TCP_LAST_ACK    9
#define TCP_TIME_WAIT  10

/* TCP flags */
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

/* TCP accept queue entry */
typedef struct tcp_accept_entry {
    tcp_conn_t* conn;
    struct tcp_accept_entry* next;
} tcp_accept_entry_t;

/* TCP connection */
typedef struct tcp_conn {
    ip_addr_t local_addr;
    u16 local_port;
    ip_addr_t remote_addr;
    u16 remote_port;
    u32 state;
    u32 seq;
    u32 ack;
    u32 send_window;
    u32 recv_window;
    socket_t* sock;
    void* recv_buffer;
    size_t recv_size;
    size_t recv_head;
    size_t recv_tail;
    u64 last_activity;      /* Last activity timestamp */
    u64 timeout_ms;         /* Connection timeout */
    tcp_accept_entry_t* accept_queue;  /* Accept queue for listen sockets */
    u32 accept_backlog;     /* Current accept queue size */
    u32 max_backlog;        /* Maximum accept queue size */
    struct tcp_conn* next;
} tcp_conn_t;

static tcp_conn_t* tcp_connections = NULL;
static spinlock_t tcp_lock = SPINLOCK_INIT;
static u16 tcp_port_counter = 1024;
static u32 tcp_seq_counter = 0;

void tcp_init(void) {
    tcp_connections = NULL;
    tcp_port_counter = 1024;
    tcp_seq_counter = 1000;
    
    /* Register TCP with IP layer */
    ip_register_protocol(IPPROTO_TCP, tcp_recv_packet);
    
    printk("[TCP] TCP protocol initialized\n");
}

static tcp_conn_t* tcp_find_connection(ip_addr_t local_addr, u16 local_port,
                                       ip_addr_t remote_addr, u16 remote_port) {
    tcp_conn_t* conn = tcp_connections;
    while (conn) {
        if (memcmp(&conn->local_addr, &local_addr, sizeof(ip_addr_t)) == 0 &&
            conn->local_port == local_port &&
            memcmp(&conn->remote_addr, &remote_addr, sizeof(ip_addr_t)) == 0 &&
            conn->remote_port == remote_port) {
            return conn;
        }
        conn = conn->next;
    }
    return NULL;
}

static int tcp_bind(socket_t* sock, const sockaddr_t* addr) {
    if (!sock || !addr) {
        return -1;
    }
    
    /* TODO: Parse sockaddr */
    u16 port = 0;
    
    spinlock_lock(&tcp_lock);
    
    /* Check if port is in use */
    tcp_conn_t* conn = tcp_connections;
    while (conn) {
        if (conn->local_port == port && conn->state == TCP_LISTEN) {
            spinlock_unlock(&tcp_lock);
            return -1; /* Port in use */
        }
        conn = conn->next;
    }
    
    spinlock_unlock(&tcp_lock);
    
    return 0;
}

static int tcp_listen(socket_t* sock, int backlog) {
    if (!sock) {
        return -1;
    }
    
    spinlock_lock(&tcp_lock);
    
    /* Create listen connection */
    tcp_conn_t* conn = (tcp_conn_t*)kzalloc(sizeof(tcp_conn_t));
    if (!conn) {
        spinlock_unlock(&tcp_lock);
        return -1;
    }
    
    conn->sock = sock;
    conn->state = TCP_LISTEN;
    conn->local_port = tcp_port_counter++;
    conn->send_window = TCP_WINDOW_SIZE;
    conn->recv_window = TCP_WINDOW_SIZE;
    conn->next = tcp_connections;
    tcp_connections = conn;
    
    spinlock_unlock(&tcp_lock);
    
    return 0;
}

static int tcp_connect(socket_t* sock, const sockaddr_t* addr) {
    if (!sock || !addr) {
        return -1;
    }
    
    /* TODO: Parse sockaddr to get remote address */
    ip_addr_t remote_addr = {0};
    u16 remote_port = 0;
    
    spinlock_lock(&tcp_lock);
    
    /* Create connection */
    tcp_conn_t* conn = (tcp_conn_t*)kzalloc(sizeof(tcp_conn_t));
    if (!conn) {
        spinlock_unlock(&tcp_lock);
        return -1;
    }
    
    conn->sock = sock;
    conn->state = TCP_SYN_SENT;
    conn->local_port = tcp_port_counter++;
    conn->remote_addr = remote_addr;
    conn->remote_port = remote_port;
    conn->seq = tcp_seq_counter++;
    conn->ack = 0;
    conn->send_window = TCP_WINDOW_SIZE;
    conn->recv_window = TCP_WINDOW_SIZE;
    conn->next = tcp_connections;
    tcp_connections = conn;
    
    spinlock_unlock(&tcp_lock);
    
    /* Send SYN */
    tcp_send_packet(conn, TCP_FLAG_SYN, NULL, 0);
    
    return 0;
}

static ssize_t tcp_send(socket_t* sock, const void* buf, size_t len) {
    if (!sock || !buf || len == 0) {
        return -1;
    }
    
    tcp_conn_t* conn = (tcp_conn_t*)sock->private_data;
    if (!conn || conn->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    /* Send data */
    tcp_send_packet(conn, TCP_FLAG_PSH | TCP_FLAG_ACK, buf, len);
    
    return len;
}

static ssize_t tcp_recv(socket_t* sock, void* buf, size_t len) {
    if (!sock || !buf || len == 0) {
        return -1;
    }
    
    tcp_conn_t* conn = (tcp_conn_t*)sock->private_data;
    if (!conn || conn->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    /* TODO: Read from receive buffer */
    (void)len;
    
    return 0;
}

static int tcp_close(socket_t* sock) {
    if (!sock) {
        return -1;
    }
    
    tcp_conn_t* conn = (tcp_conn_t*)sock->private_data;
    if (!conn) {
        return -1;
    }
    
    if (conn->state == TCP_ESTABLISHED) {
        conn->state = TCP_FIN_WAIT1;
        tcp_send_packet(conn, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
    }
    
    return 0;
}

int tcp_send_packet(tcp_conn_t* conn, u8 flags, const void* data, size_t len) {
    if (!conn) {
        return -1;
    }
    
    /* Allocate socket buffer */
    sk_buff_t* skb = skb_alloc(len + TCP_HEADER_LEN);
    if (!skb) {
        return -1;
    }
    
    skb_reserve(skb, TCP_HEADER_LEN);
    if (data && len > 0) {
        memcpy(skb_put(skb, len), data, len);
    }
    
    /* Build TCP header */
    tcp_header_t* tcph = (tcp_header_t*)skb_push(skb, TCP_HEADER_LEN);
    tcph->src_port = htons(conn->local_port);
    tcph->dst_port = htons(conn->remote_port);
    tcph->seq_num = htonl(conn->seq);
    tcph->ack_num = htonl(conn->ack);
    tcph->data_offset = (TCP_HEADER_LEN / 4) << 4;
    tcph->flags = flags;
    tcph->window = htons(conn->send_window);
    tcph->checksum = 0;
    tcph->urgent = 0;
    
    /* Update sequence number */
    if (len > 0 || (flags & TCP_FLAG_SYN) || (flags & TCP_FLAG_FIN)) {
        conn->seq += len;
        if (flags & TCP_FLAG_SYN || flags & TCP_FLAG_FIN) {
            conn->seq++;
        }
    }
    
    /* Send via IP layer */
    int ret = ip_send_packet(conn->remote_addr, IPPROTO_TCP, skb->data, skb->len);
    
    skb_free(skb);
    
    return ret;
}

int tcp_recv_packet(sk_buff_t* skb) {
    if (!skb || skb->len < TCP_HEADER_LEN) {
        return -1;
    }
    
    tcp_header_t* tcph = (tcp_header_t*)skb->data;
    u16 src_port = ntohs(tcph->src_port);
    u16 dst_port = ntohs(tcph->dst_port);
    u32 seq = ntohl(tcph->seq_num);
    u32 ack = ntohl(tcph->ack_num);
    u8 flags = tcph->flags;
    
    /* Get source IP from IP header */
    ip_addr_t src_addr = {0}; /* TODO: Extract from IP header */
    
    /* Find connection */
    spinlock_lock(&tcp_lock);
    tcp_conn_t* conn = tcp_find_connection((ip_addr_t){0}, dst_port, src_addr, src_port);
    
    if (!conn && (flags & TCP_FLAG_SYN)) {
        /* New connection - find listen socket */
        conn = tcp_connections;
        while (conn) {
            if (conn->local_port == dst_port && conn->state == TCP_LISTEN) {
                /* Create new connection */
                tcp_conn_t* new_conn = (tcp_conn_t*)kzalloc(sizeof(tcp_conn_t));
                if (new_conn) {
                    new_conn->sock = conn->sock;
                    new_conn->state = TCP_SYN_RCVD;
                    new_conn->local_port = dst_port;
                    new_conn->remote_addr = src_addr;
                    new_conn->remote_port = src_port;
                    new_conn->seq = tcp_seq_counter++;
                    new_conn->ack = seq + 1;
                    new_conn->send_window = TCP_WINDOW_SIZE;
                    new_conn->recv_window = TCP_WINDOW_SIZE;
                    new_conn->next = tcp_connections;
                    tcp_connections = new_conn;
                    
                    /* Send SYN-ACK */
                    tcp_send_packet(new_conn, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
                }
                break;
            }
            conn = conn->next;
        }
    }
    
    if (conn) {
        if (flags & TCP_FLAG_SYN && conn->state == TCP_SYN_SENT) {
            conn->state = TCP_ESTABLISHED;
            conn->ack = seq + 1;
            tcp_send_packet(conn, TCP_FLAG_ACK, NULL, 0);
        } else if (flags & TCP_FLAG_ACK && conn->state == TCP_SYN_RCVD) {
            conn->state = TCP_ESTABLISHED;
        } else if (flags & TCP_FLAG_FIN) {
            if (conn->state == TCP_ESTABLISHED) {
                conn->state = TCP_CLOSE_WAIT;
                tcp_send_packet(conn, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
            }
        } else if (flags & TCP_FLAG_ACK && (flags & TCP_FLAG_PSH)) {
            /* Data packet */
            skb_pull(skb, TCP_HEADER_LEN);
            /* TODO: Add to receive buffer */
            conn->ack = seq + skb->len;
            tcp_send_packet(conn, TCP_FLAG_ACK, NULL, 0);
        }
    }
    
    spinlock_unlock(&tcp_lock);
    
    skb_free(skb);
    return 0;
}

socket_ops_t tcp_ops = {
    .bind = tcp_bind,
    .listen = tcp_listen,
    .accept = NULL, /* TODO */
    .connect = tcp_connect,
    .send = tcp_send,
    .recv = tcp_recv,
    .close = tcp_close
};
