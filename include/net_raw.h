#ifndef NET_RAW_H
#define NET_RAW_H

#include "net.h"

extern socket_ops_t raw_ops;

void raw_init(void);
int raw_attach_socket(socket_t* sock);
int raw_recv_packet(sk_buff_t* skb);

#endif /* NET_RAW_H */
