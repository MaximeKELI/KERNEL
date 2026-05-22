#ifndef ETHERNET_H
#define ETHERNET_H

#include "types.h"

typedef int (*eth_driver_tx_t)(struct ethernet_device* dev, void* data, size_t len);
typedef int (*eth_driver_rx_t)(struct ethernet_device* dev, void* buffer, size_t buffer_size);
typedef void (*eth_driver_poll_t)(struct ethernet_device* dev);

typedef struct ethernet_device {
    u32 id;
    char name[16];
    u8 mac_address[6];
    u32 io_base;
    u32 irq;
    u32 flags;
    u64 tx_packets;
    u64 rx_packets;
    u64 tx_bytes;
    u64 rx_bytes;
    u64 tx_errors;
    u64 rx_errors;
    bool up;
    void* priv_data;
    eth_driver_tx_t tx_fn;
    eth_driver_rx_t rx_fn;
    eth_driver_poll_t poll_fn;
    struct ethernet_device* next;
} ethernet_device_t;

/* Initialize Ethernet drivers */
void ethernet_init(void);

/* Allocate/free ethernet device */
ethernet_device_t* ethernet_alloc_device(void);
void ethernet_free_device(ethernet_device_t* dev);

/* Bring device up/down */
int ethernet_up(ethernet_device_t* dev);
int ethernet_down(ethernet_device_t* dev);

/* Send/receive packets */
int ethernet_send_packet(ethernet_device_t* dev, void* data, size_t len);
int ethernet_receive_packet(ethernet_device_t* dev, void* buffer, size_t buffer_size);

/* Find device by name */
ethernet_device_t* ethernet_find_device(const char* name);

/* Get device count */
u32 ethernet_get_device_count(void);

/* Poll all hardware NICs (RX) */
void ethernet_poll_all(void);

/* Copy MAC address */
int ethernet_get_mac(ethernet_device_t* dev, u8 mac[6]);
bool ethernet_is_hardware(ethernet_device_t* dev);

#endif /* ETHERNET_H */
