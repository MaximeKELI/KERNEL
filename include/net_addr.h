#ifndef NET_ADDR_H
#define NET_ADDR_H

#include "types.h"
#include "net.h"

bool ip_addr_parse(const char* str, ip_addr_t* out);
void ip_addr_format(const ip_addr_t* ip, char* buf, size_t buflen);
bool ip_addr_same_subnet(const ip_addr_t* a, const ip_addr_t* b, const ip_addr_t* mask);
ip_addr_t ip_addr_broadcast(const ip_addr_t* ip, const ip_addr_t* mask);

#endif /* NET_ADDR_H */
