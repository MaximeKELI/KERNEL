#ifndef USB_H
#define USB_H

#include "types.h"

/* USB Device */
typedef struct usb_device usb_device_t;

/* USB Driver */
typedef struct usb_driver usb_driver_t;

/* Initialize USB Core */
void usb_init(void);

/* Register USB driver */
int usb_register_driver(usb_driver_t* driver);

/* Match driver with device */
bool usb_driver_match(usb_driver_t* driver, usb_device_t* dev);

/* Allocate/free USB device */
usb_device_t* usb_alloc_device(void);
void usb_free_device(usb_device_t* dev);

/* USB device operations */
int usb_set_address(usb_device_t* dev, u8 address);
int usb_set_configuration(usb_device_t* dev, u8 configuration);

/* Find USB device */
usb_device_t* usb_find_device(u16 vendor_id, u16 product_id);

/* Get device count */
u32 usb_get_device_count(void);

#endif /* USB_H */
