#include "tcp_cc.h"
#include "tcp.h"
#include "string.h"

/* Congestion control state lives on tcp_conn (defined in tcp.c) */
extern void tcp_cc_set_cwnd(struct tcp_conn* conn, u32 cwnd, u32 ssthresh,
                            u32 peer_rwnd, u32 flight, u32 last_ack, u8 dupacks);
extern void tcp_cc_get(struct tcp_conn* conn, u32* cwnd, u32* ssthresh,
                       u32* peer_rwnd, u32* flight, u32* last_ack, u8* dupacks);

void tcp_cc_init(struct tcp_conn* conn) {
    tcp_cc_set_cwnd(conn, TCP_INITIAL_CWND, TCP_INITIAL_SSTHRESH,
                    TCP_WINDOW_SIZE, 0, 0, 0);
}

u32 tcp_cc_snd_wnd(struct tcp_conn* conn) {
    u32 cwnd, ssthresh, peer_rwnd, flight, last_ack;
    u8 dupacks;
    tcp_cc_get(conn, &cwnd, &ssthresh, &peer_rwnd, &flight, &last_ack, &dupacks);
    (void)ssthresh;
    (void)flight;
    (void)last_ack;
    (void)dupacks;
    u32 w = cwnd;
    if (peer_rwnd < w) {
        w = peer_rwnd;
    }
    return w;
}

void tcp_cc_on_ack(struct tcp_conn* conn, u32 ack, u32 acked_bytes) {
    u32 cwnd, ssthresh, peer_rwnd, flight, last_ack;
    u8 dupacks;
    tcp_cc_get(conn, &cwnd, &ssthresh, &peer_rwnd, &flight, &last_ack, &dupacks);

    if (acked_bytes == 0 && ack == last_ack) {
        dupacks++;
        if (dupacks >= 3) {
            tcp_cc_on_loss(conn);
            dupacks = 0;
        }
    } else {
        dupacks = 0;
    }

    if (acked_bytes > 0) {
        if (cwnd < ssthresh) {
            cwnd += TCP_MSS;
            if (cwnd > ssthresh) {
                cwnd = ssthresh;
            }
        } else {
            cwnd += (TCP_MSS * TCP_MSS) / cwnd;
            if (cwnd < TCP_MSS) {
                cwnd = TCP_MSS;
            }
        }
        if (flight > acked_bytes) {
            flight -= acked_bytes;
        } else {
            flight = 0;
        }
    }

    tcp_cc_set_cwnd(conn, cwnd, ssthresh, peer_rwnd, flight, ack, dupacks);
}

void tcp_cc_on_loss(struct tcp_conn* conn) {
    u32 cwnd, ssthresh, peer_rwnd, flight, last_ack;
    u8 dupacks;
    tcp_cc_get(conn, &cwnd, &ssthresh, &peer_rwnd, &flight, &last_ack, &dupacks);
    ssthresh = cwnd / 2;
    if (ssthresh < 2 * TCP_MSS) {
        ssthresh = 2 * TCP_MSS;
    }
    cwnd = TCP_MSS;
    dupacks = 0;
    tcp_cc_set_cwnd(conn, cwnd, ssthresh, peer_rwnd, flight, last_ack, dupacks);
}

void tcp_cc_update_peer_wnd(struct tcp_conn* conn, u32 peer_wnd) {
    u32 cwnd, ssthresh, peer_rwnd_old, flight, last_ack;
    u8 dupacks;
    tcp_cc_get(conn, &cwnd, &ssthresh, &peer_rwnd_old, &flight, &last_ack, &dupacks);
    tcp_cc_set_cwnd(conn, cwnd, ssthresh, peer_wnd, flight, last_ack, dupacks);
}

int tcp_cc_parse_sack(struct tcp_conn* conn, const u8* opts, u32 optlen) {
    u32 i = 0;
    while (i + 1 < optlen) {
        u8 kind = opts[i];
        u8 len = opts[i + 1];
        if (len < 2 || i + len > optlen) {
            break;
        }
        if (kind == 5 && len >= 10) {
            /* SACK block — record for selective ack path */
            (void)conn;
        }
        if (kind == 1) {
            i++;
            continue;
        }
        i += len;
    }
    return 0;
}
