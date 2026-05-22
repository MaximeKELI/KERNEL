#ifndef ETH_H
#define ETH_H

#include "types.h"
#include "net.h"

#define ETH_ALEN 6
#define ETH_HDR_LEN 14
#define ETH_FRAME_MAX 1514

typedef struct __packed {
    u8 dst[ETH_ALEN];
    u8 src[ETH_ALEN];
    u16 proto;
} eth_header_t;

void eth_loopback_init(void);
int eth_transmit(netif_t* iface, u16 proto, const u8* dst_mac,
                 const void* payload, size_t len);
int eth_poll(netif_t* iface);

#endif /* ETH_H */
