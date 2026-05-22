#ifndef TCP_CC_H
#define TCP_CC_H

#include "types.h"

struct tcp_conn;

#define TCP_MSS           1460u
#define TCP_INITIAL_CWND  (10u * TCP_MSS)
#define TCP_INITIAL_SSTHRESH (65535u)

void tcp_cc_init(struct tcp_conn* conn);
u32 tcp_cc_snd_wnd(struct tcp_conn* conn);
void tcp_cc_on_ack(struct tcp_conn* conn, u32 ack, u32 acked_bytes);
void tcp_cc_on_loss(struct tcp_conn* conn);
void tcp_cc_update_peer_wnd(struct tcp_conn* conn, u32 peer_wnd);
int tcp_cc_parse_sack(struct tcp_conn* conn, const u8* opts, u32 optlen);

#endif /* TCP_CC_H */
