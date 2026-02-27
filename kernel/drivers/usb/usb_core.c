#include "usb.h"
#include "pci.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "memory.h"
#include "types.h"
#include "string.h"

#define MAX_USB_DEVICES 64
#define MAX_USB_DRIVERS 16

/* USB Device States */
#define USB_DEVICE_DISCONNECTED 0
#define USB_DEVICE_ATTACHED     1
#define USB_DEVICE_POWERED      2
#define USB_DEVICE_DEFAULT      3
#define USB_DEVICE_ADDRESS      4
#define USB_DEVICE_CONFIGURED   5
#define USB_DEVICE_SUSPENDED   6

/* USB Descriptor Types */
#define USB_DT_DEVICE           1
#define USB_DT_CONFIG           2
#define USB_DT_STRING           3
#define USB_DT_INTERFACE        4
#define USB_DT_ENDPOINT         5

typedef struct usb_device {
    u32 id;
    u16 vendor_id;
    u16 product_id;
    u8 device_class;
    u8 device_subclass;
    u8 device_protocol;
    u8 state;
    u8 address;
    u8 configuration;
    void* controller;
    struct usb_device* next;
} usb_device_t;

typedef struct usb_driver {
    u16 vendor_id;
    u16 product_id;
    u8 device_class;
    int (*probe)(usb_device_t* dev);
    int (*remove)(usb_device_t* dev);
    struct usb_driver* next;
} usb_driver_t;

static usb_device_t* usb_devices = NULL;
static usb_driver_t* usb_drivers = NULL;
static u32 usb_device_count = 0;
static spinlock_t usb_lock = SPINLOCK_INIT;
static bool usb_initialized = false;

void usb_init(void) {
    if (usb_initialized) {
        return;
    }
    
    usb_devices = NULL;
    usb_drivers = NULL;
    usb_device_count = 0;
    usb_initialized = true;
    
    /* Scan PCI for USB controllers */
    pci_device_t* pci_dev = pci_find_class(0x0C, 0x03); /* USB Controller */
    if (pci_dev) {
        DEBUG_INFO("USB controller found: %04x:%04x", 
                  pci_dev->vendor_id, pci_dev->device_id);
    }
    
    printk("[USB] USB Core initialized\n");
}

int usb_register_driver(usb_driver_t* driver) {
    if (!driver || !driver->probe) {
        return -1;
    }
    
    spinlock_lock(&usb_lock);
    driver->next = usb_drivers;
    usb_drivers = driver;
    spinlock_unlock(&usb_lock);
    
    /* Try to match with existing devices */
    usb_device_t* dev = usb_devices;
    while (dev) {
        if (usb_driver_match(driver, dev)) {
            driver->probe(dev);
        }
        dev = dev->next;
    }
    
    DEBUG_INFO("USB driver registered");
    return 0;
}

bool usb_driver_match(usb_driver_t* driver, usb_device_t* dev) {
    if (!driver || !dev) {
        return false;
    }
    
    /* Match by vendor/product ID */
    if (driver->vendor_id != 0 && driver->product_id != 0) {
        return (driver->vendor_id == dev->vendor_id && 
                driver->product_id == dev->product_id);
    }
    
    /* Match by device class */
    if (driver->device_class != 0) {
        return (driver->device_class == dev->device_class);
    }
    
    return false;
}

usb_device_t* usb_alloc_device(void) {
    if (usb_device_count >= MAX_USB_DEVICES) {
        return NULL;
    }
    
    usb_device_t* dev = (usb_device_t*)kzalloc(sizeof(usb_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->id = usb_device_count++;
    dev->state = USB_DEVICE_ATTACHED;
    dev->address = 0;
    dev->configuration = 0;
    
    spinlock_lock(&usb_lock);
    dev->next = usb_devices;
    usb_devices = dev;
    spinlock_unlock(&usb_lock);
    
    return dev;
}

void usb_free_device(usb_device_t* dev) {
    if (!dev) {
        return;
    }
    
    spinlock_lock(&usb_lock);
    
    /* Remove from list */
    if (usb_devices == dev) {
        usb_devices = dev->next;
    } else {
        usb_device_t* p = usb_devices;
        while (p && p->next != dev) {
            p = p->next;
        }
        if (p) {
            p->next = dev->next;
        }
    }
    
    usb_device_count--;
    spinlock_unlock(&usb_lock);
    
    kfree(dev);
}

int usb_set_address(usb_device_t* dev, u8 address) {
    if (!dev) {
        return -1;
    }
    
    dev->address = address;
    dev->state = USB_DEVICE_ADDRESS;
    
    return 0;
}

int usb_set_configuration(usb_device_t* dev, u8 configuration) {
    if (!dev) {
        return -1;
    }
    
    dev->configuration = configuration;
    dev->state = USB_DEVICE_CONFIGURED;
    
    return 0;
}

usb_device_t* usb_find_device(u16 vendor_id, u16 product_id) {
    spinlock_lock(&usb_lock);
    
    usb_device_t* dev = usb_devices;
    while (dev) {
        if (dev->vendor_id == vendor_id && dev->product_id == product_id) {
            spinlock_unlock(&usb_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&usb_lock);
    return NULL;
}

u32 usb_get_device_count(void) {
    spinlock_lock(&usb_lock);
    u32 count = usb_device_count;
    spinlock_unlock(&usb_lock);
    return count;
}
