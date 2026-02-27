#ifndef HW_OFFLOAD_H
#define HW_OFFLOAD_H

#include "types.h"
#include "skbuff.h"

/* Hardware offload capabilities */
#define HW_OFFLOAD_TCP_CSO     (1 << 0)
#define HW_OFFLOAD_UDP_CSO     (1 << 1)
#define HW_OFFLOAD_IP_CSO      (1 << 2)
#define HW_OFFLOAD_TCP_TSO     (1 << 3)
#define HW_OFFLOAD_UDP_TSO     (1 << 4)
#define HW_OFFLOAD_RX_CSUM     (1 << 5)
#define HW_OFFLOAD_VLAN_STRIP  (1 << 6)
#define HW_OFFLOAD_VLAN_INSERT (1 << 7)
#define HW_OFFLOAD_RX_HASH     (1 << 8)
#define HW_OFFLOAD_RSC         (1 << 9)
#define HW_OFFLOAD_TX_HASH     (1 << 10)

/* Hardware offload device */
typedef struct hw_offload_device hw_offload_device_t;

/* Initialize hardware offloading */
void hw_offload_init(void);

/* Enable/disable offload features */
int hw_offload_enable_feature(hw_offload_device_t* dev, u32 feature);
int hw_offload_disable_feature(hw_offload_device_t* dev, u32 feature);

/* TCP checksum offload */
int hw_offload_tcp_checksum(sk_buff_t* skb);

/* TCP segmentation offload */
int hw_offload_tcp_segmentation(sk_buff_t* skb, u32 mss);

/* Receive checksum offload */
int hw_offload_rx_checksum(sk_buff_t* skb);

/* Receive hash offload */
int hw_offload_rx_hash(sk_buff_t* skb, u32* hash);

/* Get capabilities/enabled features */
u32 hw_offload_get_capabilities(hw_offload_device_t* dev);
u32 hw_offload_get_enabled(hw_offload_device_t* dev);

/* Find device */
hw_offload_device_t* hw_offload_find_device(const char* name);

#endif /* HW_OFFLOAD_H */
