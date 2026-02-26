#include "net.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"

/* IP header */
typedef struct __packed {
    u8 version_ihl;
    u8 tos;
    u16 total_length;
    u16 id;
    u16 flags_fragment;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    ip_addr_t src;
    ip_addr_t dst;
} ip_header_t;

int ip_send_packet(ip_addr_t dst, u8 protocol, const void* data, size_t len) {
    (void)dst;
    (void)protocol;
    (void)data;
    (void)len;
    /* Would send IP packet */
    return 0;
}

int ip_recv_packet(void* data, size_t* len) {
    (void)data;
    (void)len;
    /* Would receive IP packet */
    return 0;
}
