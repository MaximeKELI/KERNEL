#include "ethernet.h"
#include "pci.h"
#include "net.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "memory.h"
#include "types.h"
#include "string.h"

#define MAX_ETHERNET_DEVICES 8
#define ETH_ALEN 6  /* Ethernet address length */

/* Ethernet device */
typedef struct ethernet_device {
    u32 id;
    char name[16];
    u8 mac_address[ETH_ALEN];
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
    struct ethernet_device* next;
} ethernet_device_t;

static ethernet_device_t* ethernet_devices = NULL;
static u32 ethernet_device_count = 0;
static spinlock_t ethernet_lock = SPINLOCK_INIT;
static bool ethernet_initialized = false;

void ethernet_init(void) {
    if (ethernet_initialized) {
        return;
    }
    
    ethernet_devices = NULL;
    ethernet_device_count = 0;
    ethernet_initialized = true;
    
    /* Scan PCI for Ethernet controllers */
    pci_device_t* pci_dev = pci_find_class(0x02, 0x00); /* Ethernet Controller */
    if (pci_dev) {
        DEBUG_INFO("Ethernet controller found: %04x:%04x", 
                  pci_dev->vendor_id, pci_dev->device_id);
        
        /* Create ethernet device */
        ethernet_device_t* dev = ethernet_alloc_device();
        if (dev) {
            dev->io_base = pci_dev->bar[0] & ~0xF;
            dev->irq = 11; /* Default IRQ */
            snprintf(dev->name, sizeof(dev->name), "eth%d", dev->id);
            
            /* Read MAC address (would read from hardware) */
            memset(dev->mac_address, 0, ETH_ALEN);
            dev->mac_address[0] = 0x02;
            dev->mac_address[1] = 0x00;
            dev->mac_address[2] = 0x00;
            dev->mac_address[3] = 0x00;
            dev->mac_address[4] = 0x00;
            dev->mac_address[5] = dev->id;
            
            DEBUG_INFO("Ethernet device created: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                      dev->name,
                      dev->mac_address[0], dev->mac_address[1], dev->mac_address[2],
                      dev->mac_address[3], dev->mac_address[4], dev->mac_address[5]);
        }
    }
    
    printk("[Ethernet] Ethernet drivers initialized\n");
}

ethernet_device_t* ethernet_alloc_device(void) {
    if (ethernet_device_count >= MAX_ETHERNET_DEVICES) {
        return NULL;
    }
    
    ethernet_device_t* dev = (ethernet_device_t*)kzalloc(sizeof(ethernet_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->id = ethernet_device_count++;
    dev->up = false;
    dev->tx_packets = 0;
    dev->rx_packets = 0;
    dev->tx_bytes = 0;
    dev->rx_bytes = 0;
    dev->tx_errors = 0;
    dev->rx_errors = 0;
    
    spinlock_lock(&ethernet_lock);
    dev->next = ethernet_devices;
    ethernet_devices = dev;
    spinlock_unlock(&ethernet_lock);
    
    return dev;
}

void ethernet_free_device(ethernet_device_t* dev) {
    if (!dev) {
        return;
    }
    
    spinlock_lock(&ethernet_lock);
    
    if (ethernet_devices == dev) {
        ethernet_devices = dev->next;
    } else {
        ethernet_device_t* p = ethernet_devices;
        while (p && p->next != dev) {
            p = p->next;
        }
        if (p) {
            p->next = dev->next;
        }
    }
    
    ethernet_device_count--;
    spinlock_unlock(&ethernet_lock);
    
    kfree(dev);
}

int ethernet_up(ethernet_device_t* dev) {
    if (!dev) {
        return -1;
    }
    
    dev->up = true;
    DEBUG_INFO("Ethernet device %s brought up", dev->name);
    return 0;
}

int ethernet_down(ethernet_device_t* dev) {
    if (!dev) {
        return -1;
    }
    
    dev->up = false;
    DEBUG_INFO("Ethernet device %s brought down", dev->name);
    return 0;
}

int ethernet_send_packet(ethernet_device_t* dev, void* data, size_t len) {
    if (!dev || !dev->up || !data || len == 0) {
        return -1;
    }
    
    /* TODO: Send packet to hardware */
    dev->tx_packets++;
    dev->tx_bytes += len;
    
    return 0;
}

int ethernet_receive_packet(ethernet_device_t* dev, void* buffer, size_t buffer_size) {
    if (!dev || !dev->up || !buffer || buffer_size == 0) {
        return -1;
    }
    
    /* TODO: Receive packet from hardware */
    dev->rx_packets++;
    dev->rx_bytes += buffer_size;
    
    return 0;
}

ethernet_device_t* ethernet_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&ethernet_lock);
    
    ethernet_device_t* dev = ethernet_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&ethernet_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&ethernet_lock);
    return NULL;
}

u32 ethernet_get_device_count(void) {
    spinlock_lock(&ethernet_lock);
    u32 count = ethernet_device_count;
    spinlock_unlock(&ethernet_lock);
    return count;
}
