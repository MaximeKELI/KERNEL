#ifndef ETHERNET_H
#define ETHERNET_H

#include "types.h"

/* Ethernet device */
typedef struct ethernet_device ethernet_device_t;

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

/* Copy MAC address */
int ethernet_get_mac(ethernet_device_t* dev, u8 mac[6]);

#endif /* ETHERNET_H */
