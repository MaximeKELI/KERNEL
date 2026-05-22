#include "ethernet.h"
#include "eth.h"
#include "pci.h"
#include "virtio.h"
#include "rtl8139.h"
#include "net.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "memory.h"
#include "types.h"
#include "string.h"

#define MAX_ETHERNET_DEVICES 8
#define ETH_ALEN 6  /* Ethernet address length */

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
    eth_loopback_init();
    ethernet_initialized = true;
    
    ethernet_device_t* dev = ethernet_alloc_device();
    if (dev) {
        snprintf(dev->name, sizeof(dev->name), "eth%d", dev->id);
        bool nic_ok = false;

        pci_device_t* pci = pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_DEVICE_NET);
        if (!pci) {
            pci = pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_DEVICE_NET_TRANS);
        }
        if (pci && virtio_net_probe(pci, dev) == 0) {
            nic_ok = true;
        } else {
            pci = pci_find_device(RTL8139_VENDOR, RTL8139_DEVICE);
            if (pci && rtl8139_probe(pci, dev) == 0) {
                nic_ok = true;
            }
        }

        if (!nic_ok) {
            dev->mac_address[0] = 0x02;
            dev->mac_address[1] = 0x00;
            dev->mac_address[2] = 0x00;
            dev->mac_address[3] = 0x00;
            dev->mac_address[4] = 0x00;
            dev->mac_address[5] = (u8)(0x10 + dev->id);
            printk("[Ethernet] No virtio/rtl8139 — loopback only\n");
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

void ethernet_poll_all(void) {
    spinlock_lock(&ethernet_lock);
    ethernet_device_t* dev = ethernet_devices;
    while (dev) {
        if (dev->up && dev->poll_fn) {
            dev->poll_fn(dev);
        }
        dev = dev->next;
    }
    spinlock_unlock(&ethernet_lock);
}

int ethernet_send_packet(ethernet_device_t* dev, void* data, size_t len) {
    if (!dev || !dev->up || !data || len == 0) {
        return -1;
    }

    if (dev->tx_fn) {
        return dev->tx_fn(dev, data, len);
    }

    if (eth_loop_enqueue(dev->name, data, len) < 0) {
        dev->tx_errors++;
        return -1;
    }

    dev->tx_packets++;
    dev->tx_bytes += len;
    return 0;
}

int ethernet_receive_packet(ethernet_device_t* dev, void* buffer, size_t buffer_size) {
    if (!dev || !dev->up || !buffer || buffer_size == 0) {
        return -1;
    }

    if (dev->rx_fn) {
        return dev->rx_fn(dev, buffer, buffer_size);
    }

    int n = eth_loop_dequeue(dev->name, buffer, buffer_size);
    if (n > 0) {
        dev->rx_packets++;
        dev->rx_bytes += (u64)n;
    }
    return n;
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

int ethernet_get_mac(ethernet_device_t* dev, u8 mac[6]) {
    if (!dev || !mac) {
        return -1;
    }
    memcpy(mac, dev->mac_address, ETH_ALEN);
    return 0;
}

bool ethernet_is_hardware(ethernet_device_t* dev) {
    return dev && dev->tx_fn != NULL;
}

u32 ethernet_get_device_count(void) {
    spinlock_lock(&ethernet_lock);
    u32 count = ethernet_device_count;
    spinlock_unlock(&ethernet_lock);
    return count;
}
