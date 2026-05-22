#include "net_addr.h"
#include "string.h"
#include "stdio.h"

static int parse_u8(const char** p, u8* out) {
    u32 val = 0;
    if (!**p) {
        return -1;
    }
    while (**p >= '0' && **p <= '9') {
        val = val * 10 + (u32)(**p - '0');
        if (val > 255) {
            return -1;
        }
        (*p)++;
    }
    *out = (u8)val;
    return 0;
}

bool ip_addr_parse(const char* str, ip_addr_t* out) {
    if (!str || !out) {
        return false;
    }
    const char* p = str;
    for (int i = 0; i < 4; i++) {
        if (parse_u8(&p, &out->addr[i]) < 0) {
            return false;
        }
        if (i < 3) {
            if (*p != '.') {
                return false;
            }
            p++;
        }
    }
    return *p == '\0';
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
