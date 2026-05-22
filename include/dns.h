#ifndef DNS_H
#define DNS_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

void dns_init(void);
int dns_resolve_a(const char* hostname, ip_addr_t* out);
int dns_handle_reply(sk_buff_t* skb, u16 src_port);
void dns_set_server(ip_addr_t server);

#endif /* DNS_H */
