#ifndef KERNEL_BYPASS_H
#define KERNEL_BYPASS_H

#include "types.h"
#include "ethernet.h"

/* Bypass device */
typedef struct bypass_device bypass_device_t;

/* Initialize kernel bypass */
void kernel_bypass_init(void);

/* Create/destroy bypass device */
bypass_device_t* kernel_bypass_create_device(const char* name, 
                                             ethernet_device_t* eth_dev, 
                                             u32 num_queues);
void kernel_bypass_destroy_device(bypass_device_t* dev);

/* Enable/disable bypass */
int kernel_bypass_enable(bypass_device_t* dev);
int kernel_bypass_disable(bypass_device_t* dev);

/* Receive/transmit bursts (zero-copy) */
u32 kernel_bypass_rx_burst(bypass_device_t* dev, u32 queue_id,
                           void** packets, u32 max_packets);
u32 kernel_bypass_tx_burst(bypass_device_t* dev, u32 queue_id,
                           void** packets, u32* lengths, u32 num_packets);

/* Get statistics */
void kernel_bypass_get_stats(bypass_device_t* dev, u32 queue_id,
                             u64* rx_packets, u64* tx_packets,
                             u64* rx_bytes, u64* tx_bytes);

/* Find device */
bypass_device_t* kernel_bypass_find_device(const char* name);

#endif /* KERNEL_BYPASS_H */
