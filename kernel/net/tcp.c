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
#include "validate.h"
#include "drivers/timer.h"

#define TCP_HEADER_LEN 20
#define TCP_INITIAL_RTO_MS 1000
#define TCP_MAX_RETRIES 12
#define TCP_MSL_MS 60000
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
    u32 snd_una;            /* oldest unacked seq */
    u32 snd_nxt;            /* next seq to send */
    u32 rto_ms;
    u64 retransmit_at;
    u8 retry_count;
    u8 last_flags;
    size_t last_len;
    u8 last_payload[1460];
    struct tcp_conn* next;
} tcp_conn_t;

static tcp_conn_t* tcp_connections = NULL;
static spinlock_t tcp_lock = SPINLOCK_INIT;
static u16 tcp_port_counter = 1024;
static u32 tcp_seq_counter = 0;
static u32 tcp_connection_count = 0;

/**
 * @brief Initialize TCP protocol layer
 * 
 * Initializes the TCP protocol subsystem, including:
 * - Connection list
 * - Port counter
 * - Sequence number counter
 * - Registration with IP layer
 * 
 * @note Must be called before any TCP operations
 */
void tcp_init(void) {
    tcp_connections = NULL;
    tcp_port_counter = 1024;
    tcp_seq_counter = 1000;
    
    /* Register TCP with IP layer */
    ip_register_protocol(IPPROTO_TCP, tcp_recv_packet);
    printk("[TCP] RFC-style stack (RTO/retransmit/time-wait)\n");
}

static u64 tcp_now_ms(void) {
    return timer_get_ticks();
}

static void tcp_arm_retransmit(tcp_conn_t* conn) {
    if (!conn) {
        return;
    }
    conn->retransmit_at = tcp_now_ms() + conn->rto_ms;
}

static int tcp_retransmit_last(tcp_conn_t* conn) {
    if (!conn) {
        return -1;
    }
    return tcp_send_packet(conn, conn->last_flags,
                           conn->last_len ? conn->last_payload : NULL,
                           conn->last_len);
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
    
    /* sockaddr structure: sa_family (2 bytes) + sa_data (14 bytes) */
    /* For AF_INET: sa_data contains port (2 bytes) + IP (4 bytes) + padding */
    if (addr->sa_family != 2) { /* AF_INET = 2 */
        return -1;
    }
    
    /* Extract port (network byte order) */
    *port = ntohs(*(u16*)&addr->sa_data[0]);
    
    /* Extract IP address */
    memcpy(ip, &addr->sa_data[2], 4);
    
    return 0;
}

static int tcp_bind(socket_t* sock, const sockaddr_t* addr) {
    if (!sock || !addr) {
        return -1;
    }
    
    ip_addr_t bind_ip = {0};
    u16 port = 0;
    
    if (parse_sockaddr(addr, &bind_ip, &port) < 0) {
        return -1;
    }
    
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
    
    sock->local_addr = bind_ip;
    sock->local_port = port;
    
    spinlock_unlock(&tcp_lock);
    
    return 0;
}

static int tcp_listen(socket_t* sock, int backlog) {
    if (!sock) {
        return -1;
    }
    
    if (backlog < 1) {
        backlog = 1;
    }
    if (backlog > TCP_ACCEPT_BACKLOG_MAX) {
        backlog = TCP_ACCEPT_BACKLOG_MAX;
    }
    
    spinlock_lock(&tcp_lock);
    
    /* Check connection limit */
    if (tcp_connection_count >= MAX_TCP_CONNECTIONS) {
        spinlock_unlock(&tcp_lock);
        return -1;
    }
    
    /* Create listen connection */
    tcp_conn_t* conn = (tcp_conn_t*)kzalloc(sizeof(tcp_conn_t));
    if (!conn) {
        spinlock_unlock(&tcp_lock);
        return -1;
    }
    
    conn->sock = sock;
    conn->state = TCP_LISTEN;
    conn->local_addr = sock->local_addr;
    conn->local_port = sock->local_port ? sock->local_port : tcp_port_counter++;
    conn->send_window = TCP_WINDOW_SIZE;
    conn->recv_window = TCP_WINDOW_SIZE;
    conn->accept_queue = NULL;
    conn->accept_backlog = 0;
    conn->max_backlog = backlog;
    conn->timeout_ms = TCP_DEFAULT_TIMEOUT_MS;
    conn->last_activity = 0; /* TODO: Use actual timestamp */
    conn->next = tcp_connections;
    tcp_connections = conn;
    tcp_connection_count++;
    
    sock->private_data = conn;
    
    spinlock_unlock(&tcp_lock);
    
    return 0;
}

static int tcp_connect(socket_t* sock, const sockaddr_t* addr) {
    if (!sock || !addr) {
        return -1;
    }
    
    ip_addr_t remote_addr = {0};
    u16 remote_port = 0;
    
    if (parse_sockaddr(addr, &remote_addr, &remote_port) < 0) {
        return -1;
    }
    
    spinlock_lock(&tcp_lock);
    
    /* Check connection limit */
    if (tcp_connection_count >= MAX_TCP_CONNECTIONS) {
        spinlock_unlock(&tcp_lock);
        return -1;
    }
    
    /* Create connection */
    tcp_conn_t* conn = (tcp_conn_t*)kzalloc(sizeof(tcp_conn_t));
    if (!conn) {
        spinlock_unlock(&tcp_lock);
        return -1;
    }
    
    conn->sock = sock;
    conn->state = TCP_SYN_SENT;
    conn->local_addr = sock->local_addr;
    conn->local_port = sock->local_port ? sock->local_port : tcp_port_counter++;
    conn->remote_addr = remote_addr;
    conn->remote_port = remote_port;
    conn->seq = tcp_seq_counter++;
    conn->ack = 0;
    conn->send_window = TCP_WINDOW_SIZE;
    conn->recv_window = TCP_WINDOW_SIZE;
    conn->recv_buffer = kzalloc(TCP_RECV_BUFFER_SIZE);
    conn->recv_size = TCP_RECV_BUFFER_SIZE;
    conn->recv_head = 0;
    conn->recv_tail = 0;
    conn->timeout_ms = TCP_DEFAULT_TIMEOUT_MS;
    conn->last_activity = 0; /* TODO: Use actual timestamp */
    conn->next = tcp_connections;
    tcp_connections = conn;
    tcp_connection_count++;
    
    sock->private_data = conn;
    sock->remote_addr = remote_addr;
    sock->remote_port = remote_port;
    
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
    
    if (!conn->recv_buffer) {
        return -1;
    }
    
    spinlock_lock(&tcp_lock);
    
    /* Calculate available data */
    size_t available = 0;
    if (conn->recv_tail >= conn->recv_head) {
        available = conn->recv_tail - conn->recv_head;
    } else {
        available = conn->recv_size - conn->recv_head + conn->recv_tail;
    }
    
    if (available == 0) {
        spinlock_unlock(&tcp_lock);
        return 0; /* No data available */
    }
    
    /* Limit to requested length */
    if (len > available) {
        len = available;
    }
    
    /* Copy data from receive buffer */
    u8* recv_buf = (u8*)conn->recv_buffer;
    size_t copied = 0;
    
    while (copied < len && available > 0) {
        size_t to_copy = len - copied;
        if (conn->recv_head + to_copy > conn->recv_size) {
            to_copy = conn->recv_size - conn->recv_head;
        }
        if (to_copy > available) {
            to_copy = available;
        }
        
        memcpy((u8*)buf + copied, recv_buf + conn->recv_head, to_copy);
        conn->recv_head = (conn->recv_head + to_copy) % conn->recv_size;
        copied += to_copy;
        available -= to_copy;
    }
    
    spinlock_unlock(&tcp_lock);
    
    return copied;
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

/**
 * @brief Send a TCP packet
 * @param conn TCP connection structure
 * @param flags TCP flags (SYN, ACK, FIN, etc.)
 * @param data Packet data payload
 * @param len Length of data payload
 * @return 0 on success, -1 on error
 * 
 * Builds and sends a TCP packet with the specified flags and data.
 * Updates sequence numbers automatically.
 */
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
    conn->last_flags = flags;
    conn->last_len = len;
    if (data && len > 0 && len <= sizeof(conn->last_payload)) {
        memcpy(conn->last_payload, data, len);
    }

    if (len > 0 || (flags & TCP_FLAG_SYN) || (flags & TCP_FLAG_FIN)) {
        u32 seq_inc = (u32)len;
        if (flags & TCP_FLAG_SYN || flags & TCP_FLAG_FIN) {
            seq_inc++;
        }
        conn->seq += seq_inc;
        conn->snd_nxt = conn->seq;
    }

    tcp_arm_retransmit(conn);

    /* Send via IP layer */
    int ret = ip_send_packet(conn->remote_addr, IPPROTO_TCP, skb->data, skb->len);
    
    skb_free(skb);
    
    return ret;
}

/**
 * @brief Receive and process a TCP packet from IP layer
 * @param skb Socket buffer containing the TCP packet
 * @return 0 on success, -1 on error
 * 
 * Processes incoming TCP packets, handling:
 * - Connection establishment (SYN, SYN-ACK, ACK)
 * - Data reception (PSH+ACK)
 * - Connection termination (FIN)
 * - Accept queue management for listening sockets
 */
int tcp_recv_packet(sk_buff_t* skb) {
    if (!skb || skb->len < TCP_HEADER_LEN) {
        return -1;
    }
    
    tcp_header_t* tcph = (tcp_header_t*)skb->data;
    if (skb->ip_hdr && !tcp_udp_checksum_valid(skb->ip_hdr, skb->data, skb->len, IPPROTO_TCP)) {
        DEBUG_ERROR("Invalid TCP checksum");
        skb_free(skb);
        return -1;
    }
    u16 src_port = ntohs(tcph->src_port);
    u16 dst_port = ntohs(tcph->dst_port);
    u32 seq = ntohl(tcph->seq_num);
    u32 ack = ntohl(tcph->ack_num);
    u8 flags = tcph->flags;
    
    /* Get source IP from IP header stored in skb */
    ip_addr_t src_addr = {0};
    if (skb->ip_hdr) {
        src_addr = skb->ip_hdr->src;
    }
    
    /* Find connection */
    spinlock_lock(&tcp_lock);
    tcp_conn_t* conn = tcp_find_connection((ip_addr_t){0}, dst_port, src_addr, src_port);
    
    if (!conn && (flags & TCP_FLAG_SYN)) {
        /* New connection - find listen socket */
        tcp_conn_t* listen_conn = tcp_connections;
        while (listen_conn) {
            if (listen_conn->local_port == dst_port && listen_conn->state == TCP_LISTEN) {
                /* Check accept queue limit */
                if (listen_conn->accept_backlog >= listen_conn->max_backlog) {
                    spinlock_unlock(&tcp_lock);
                    skb_free(skb);
                    return -1; /* Accept queue full */
                }
                
                /* Check connection limit */
                if (tcp_connection_count >= MAX_TCP_CONNECTIONS) {
                    spinlock_unlock(&tcp_lock);
                    skb_free(skb);
                    return -1; /* Too many connections */
                }
                
                /* Create new connection */
                tcp_conn_t* new_conn = (tcp_conn_t*)kzalloc(sizeof(tcp_conn_t));
                if (new_conn) {
                    new_conn->sock = listen_conn->sock;
                    new_conn->state = TCP_SYN_RCVD;
                    new_conn->local_addr = listen_conn->local_addr;
                    new_conn->local_port = dst_port;
                    new_conn->remote_addr = src_addr;
                    new_conn->remote_port = src_port;
                    new_conn->seq = tcp_seq_counter++;
                    new_conn->ack = seq + 1;
                    new_conn->send_window = TCP_WINDOW_SIZE;
                    new_conn->recv_window = TCP_WINDOW_SIZE;
                    new_conn->recv_buffer = kzalloc(TCP_RECV_BUFFER_SIZE);
                    new_conn->recv_size = TCP_RECV_BUFFER_SIZE;
                    new_conn->recv_head = 0;
                    new_conn->recv_tail = 0;
                    new_conn->timeout_ms = TCP_DEFAULT_TIMEOUT_MS;
                    new_conn->last_activity = 0; /* TODO: Use actual timestamp */
                    new_conn->next = tcp_connections;
                    tcp_connections = new_conn;
                    tcp_connection_count++;
                    
                    /* Add to accept queue */
                    tcp_accept_entry_t* entry = (tcp_accept_entry_t*)kzalloc(sizeof(tcp_accept_entry_t));
                    if (entry) {
                        entry->conn = new_conn;
                        entry->next = listen_conn->accept_queue;
                        listen_conn->accept_queue = entry;
                        listen_conn->accept_backlog++;
                    }
                    
                    /* Send SYN-ACK */
                    tcp_send_packet(new_conn, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
                }
                break;
            }
            listen_conn = listen_conn->next;
        }
    }
    
    if (conn) {
        /* Update last activity */
        conn->last_activity = 0; /* TODO: Use actual timestamp */
        
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
            
            /* Add to receive buffer */
            if (conn->recv_buffer && skb->len > 0) {
                u8* recv_buf = (u8*)conn->recv_buffer;
                size_t data_len = skb->len;
                size_t free_space = 0;
                
                if (conn->recv_tail >= conn->recv_head) {
                    free_space = conn->recv_size - conn->recv_tail;
                } else {
                    free_space = conn->recv_head - conn->recv_tail;
                }
                
                if (data_len <= free_space) {
                    size_t to_copy = data_len;
                    if (conn->recv_tail + to_copy > conn->recv_size) {
                        size_t first_part = conn->recv_size - conn->recv_tail;
                        memcpy(recv_buf + conn->recv_tail, skb->data, first_part);
                        memcpy(recv_buf, skb->data + first_part, to_copy - first_part);
                        conn->recv_tail = to_copy - first_part;
                    } else {
                        memcpy(recv_buf + conn->recv_tail, skb->data, to_copy);
                        conn->recv_tail = (conn->recv_tail + to_copy) % conn->recv_size;
                    }
                }
            }
            
            conn->ack = seq + skb->len;
            tcp_send_packet(conn, TCP_FLAG_ACK, NULL, 0);
        }
    }
    
    spinlock_unlock(&tcp_lock);
    
    skb_free(skb);
    return 0;
}

/**
 * @brief Accept a new connection on a listening socket
 * @param sock Listening socket
 * @param addr Output address of accepted connection
 * @return New socket on success, NULL on error
 */
static socket_t* tcp_accept(socket_t* sock, sockaddr_t* addr) {
    if (!sock) {
        return NULL;
    }
    
    tcp_conn_t* listen_conn = (tcp_conn_t*)sock->private_data;
    if (!listen_conn || listen_conn->state != TCP_LISTEN) {
        return NULL;
    }
    
    spinlock_lock(&tcp_lock);
    
    /* Check if there are pending connections */
    if (!listen_conn->accept_queue || listen_conn->accept_backlog == 0) {
        spinlock_unlock(&tcp_lock);
        return NULL; /* No pending connections */
    }
    
    /* Get first connection from accept queue */
    tcp_accept_entry_t* entry = listen_conn->accept_queue;
    tcp_conn_t* new_conn = entry->conn;
    listen_conn->accept_queue = entry->next;
    listen_conn->accept_backlog--;
    kfree(entry);
    
    /* Create new socket for accepted connection */
    socket_t* new_sock = socket_create(2, SOCK_STREAM, IPPROTO_TCP); /* AF_INET = 2 */
    if (!new_sock) {
        spinlock_unlock(&tcp_lock);
        return NULL;
    }
    
    new_sock->private_data = new_conn;
    new_sock->local_addr = new_conn->local_addr;
    new_sock->local_port = new_conn->local_port;
    new_sock->remote_addr = new_conn->remote_addr;
    new_sock->remote_port = new_conn->remote_port;
    new_sock->ops = &tcp_ops;
    
    new_conn->sock = new_sock;
    
    /* Fill in address if provided */
    if (addr) {
        addr->sa_family = 2; /* AF_INET */
        *(u16*)&addr->sa_data[0] = htons(new_conn->remote_port);
        memcpy(&addr->sa_data[2], &new_conn->remote_addr, 4);
    }
    
    spinlock_unlock(&tcp_lock);
    
    return new_sock;
}

bool tcp_socket_established(socket_t* sock) {
    if (!sock) {
        return false;
    }
    tcp_conn_t* conn = (tcp_conn_t*)sock->private_data;
    return conn && conn->state == TCP_ESTABLISHED;
}

socket_ops_t tcp_ops = {
    .bind = tcp_bind,
    .listen = tcp_listen,
    .accept = tcp_accept,
    .connect = tcp_connect,
    .send = tcp_send,
    .recv = tcp_recv,
    .close = tcp_close
};
