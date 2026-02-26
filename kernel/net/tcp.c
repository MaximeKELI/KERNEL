#include "net.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"

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

/* TCP header */
typedef struct __packed {
    u16 src_port;
    u16 dst_port;
    u32 seq_num;
    u32 ack_num;
    u8 data_offset;
    u8 flags;
    u16 window;
    u16 checksum;
    u16 urgent;
} tcp_header_t;

/* TCP connection */
typedef struct tcp_conn {
    ip_addr_t local_addr;
    u16 local_port;
    ip_addr_t remote_addr;
    u16 remote_port;
    u32 state;
    u32 seq;
    u32 ack;
    void* recv_buffer;
    size_t recv_size;
    struct tcp_conn* next;
} tcp_conn_t;

static tcp_conn_t* tcp_connections = NULL;

static int tcp_bind(socket_t* sock, const sockaddr_t* addr) {
    (void)sock;
    (void)addr;
    return 0;
}

static int tcp_listen(socket_t* sock, int backlog) {
    (void)sock;
    (void)backlog;
    return 0;
}

static int tcp_accept(socket_t* sock, sockaddr_t* addr) {
    (void)sock;
    (void)addr;
    return 0;
}

static int tcp_connect(socket_t* sock, const sockaddr_t* addr) {
    (void)sock;
    (void)addr;
    return 0;
}

static ssize_t tcp_send(socket_t* sock, const void* buf, size_t len) {
    (void)sock;
    (void)buf;
    (void)len;
    return 0;
}

static ssize_t tcp_recv(socket_t* sock, void* buf, size_t len) {
    (void)sock;
    (void)buf;
    (void)len;
    return 0;
}

static int tcp_close(socket_t* sock) {
    (void)sock;
    return 0;
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
