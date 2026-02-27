#ifndef TCP_H
#define TCP_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

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
typedef struct tcp_conn tcp_conn_t;

/* Initialize TCP */
void tcp_init(void);

/* Send TCP packet */
int tcp_send_packet(tcp_conn_t* conn, u8 flags, const void* data, size_t len);

/* Receive TCP packet from IP layer */
int tcp_recv_packet(sk_buff_t* skb);

/* TCP socket operations */
extern socket_ops_t tcp_ops;

#endif /* TCP_H */
