#include "net_addr.h"
#include "string.h"
#include "stdio.h"

bool ip_addr_parse(const char* str, ip_addr_t* out) {
    if (!str || !out) {
        return false;
    }
    unsigned int a = 0, b = 0, c = 0, d = 0;
    if (sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return false;
    }
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
    }
    out->addr[0] = (u8)a;
    out->addr[1] = (u8)b;
    out->addr[2] = (u8)c;
    out->addr[3] = (u8)d;
    return true;
}

void ip_addr_format(const ip_addr_t* ip, char* buf, size_t buflen) {
    if (!ip || !buf || buflen < 8) {
        return;
    }
    snprintf(buf, buflen, "%u.%u.%u.%u",
             ip->addr[0], ip->addr[1], ip->addr[2], ip->addr[3]);
}

bool ip_addr_same_subnet(const ip_addr_t* a, const ip_addr_t* b, const ip_addr_t* mask) {
    if (!a || !b || !mask) {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        if ((a->addr[i] & mask->addr[i]) != (b->addr[i] & mask->addr[i])) {
            return false;
        }
    }
    return true;
}

ip_addr_t ip_addr_broadcast(const ip_addr_t* ip, const ip_addr_t* mask) {
    ip_addr_t b = {{0, 0, 0, 0}};
    if (!ip || !mask) {
        return b;
    }
    for (int i = 0; i < 4; i++) {
        b.addr[i] = (ip->addr[i] & mask->addr[i]) | (~mask->addr[i]);
    }
    return b;
}
