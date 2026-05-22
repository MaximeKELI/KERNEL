#ifndef NET_RAW_H
#define NET_RAW_H

#include "net.h"

extern socket_ops_t raw_ops;

int raw_recv_packet(sk_buff_t* skb);

#endif /* NET_RAW_H */
