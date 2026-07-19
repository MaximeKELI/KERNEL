#include "hw_offload.h"
#include "net.h"
#include "tcp.h"
#include "ip.h"
#include "ethernet.h"
#include "memory.h"
#include "pci.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "io.h"

#define MAX_OFFLOAD_DEVICES 16
#define OFFLOAD_CAP_TCP_CSO     (1 << 0)  /* TCP Checksum Offload */
#define OFFLOAD_CAP_UDP_CSO     (1 << 1)  /* UDP Checksum Offload */
#define OFFLOAD_CAP_IP_CSO      (1 << 2)  /* IP Checksum Offload */
#define OFFLOAD_CAP_TCP_TSO     (1 << 3)  /* TCP Segmentation Offload */
#define OFFLOAD_CAP_UDP_TSO     (1 << 4)  /* UDP Fragmentation Offload */
#define OFFLOAD_CAP_RX_CSUM     (1 << 5)  /* Receive Checksum */
#define OFFLOAD_CAP_VLAN_STRIP  (1 << 6)  /* VLAN Strip */
#define OFFLOAD_CAP_VLAN_INSERT (1 << 7)  /* VLAN Insert */
#define OFFLOAD_CAP_RX_HASH     (1 << 8)  /* Receive Hash */
#define OFFLOAD_CAP_RSC         (1 << 9)  /* Receive Side Coalescing */
#define OFFLOAD_CAP_TX_HASH     (1 << 10) /* Transmit Hash */

/* Hardware offload device */
typedef struct hw_offload_device {
    u32 device_id;
    char name[32];
    pci_device_t* pci_dev;
    void* mmio_base;
    u32 capabilities;
    u32 enabled_features;
    bool active;
    struct hw_offload_device* next;
} hw_offload_device_t;

static hw_offload_device_t* offload_devices = NULL;
static spinlock_t offload_lock = SPINLOCK_INIT;

void hw_offload_init(void) {
    offload_devices = NULL;
    
    /* Scan PCI for network devices with offload capabilities */
    pci_device_t* pci_dev = pci_find_class(0x02, 0x00); /* Ethernet */
    while (pci_dev) {
        /* Check for offload capabilities */
        u32 capabilities = 0;
        
        /* Read device capabilities (simplified) */
        /* In real implementation, would read from PCI config space */
        capabilities = OFFLOAD_CAP_TCP_CSO | OFFLOAD_CAP_UDP_CSO | 
                      OFFLOAD_CAP_IP_CSO | OFFLOAD_CAP_TCP_TSO |
                      OFFLOAD_CAP_RX_CSUM | OFFLOAD_CAP_RX_HASH;
        
        if (capabilities != 0) {
            hw_offload_device_t* dev = (hw_offload_device_t*)kzalloc(sizeof(hw_offload_device_t));
            if (dev) {
                dev->device_id = 0; /* TODO: Generate ID */
                snprintf(dev->name, sizeof(dev->name), "offload%d", dev->device_id);
                dev->pci_dev = pci_dev;
                dev->mmio_base = (void*)(pci_dev->bar[0] & ~0xF);
                dev->capabilities = capabilities;
                dev->enabled_features = 0;
                dev->active = true;
                
                spinlock_lock(&offload_lock);
                dev->next = offload_devices;
                offload_devices = dev;
                spinlock_unlock(&offload_lock);
                
                DEBUG_INFO("Hardware offload device found: %s, caps=0x%x", 
                          dev->name, capabilities);
            }
        }
        
        pci_dev = pci_dev->next;
    }
    
    printk("[HW Offload] Hardware offloading initialized\n");
}

int hw_offload_enable_feature(hw_offload_device_t* dev, u32 feature) {
    if (!dev || !(dev->capabilities & feature)) {
        return -1;
    }
    
    dev->enabled_features |= feature;
    
    /* Enable in hardware */
    /* TODO: Write to NIC registers */
    
    DEBUG_INFO("Hardware offload feature enabled: 0x%x", feature);
    return 0;
}

int hw_offload_disable_feature(hw_offload_device_t* dev, u32 feature) {
    if (!dev) {
        return -1;
    }
    
    dev->enabled_features &= ~feature;
    
    /* Disable in hardware */
    /* TODO: Write to NIC registers */
    
    return 0;
}

int hw_offload_tcp_checksum(sk_buff_t* skb) {
    if (!skb || skb->len < sizeof(tcp_header_t)) {
        return -1;
    }
    
    /* Find offload device */
    hw_offload_device_t* dev = offload_devices;
    if (!dev || !(dev->enabled_features & OFFLOAD_CAP_TCP_CSO)) {
        return -1;
    }
    
    /* Hardware will calculate checksum */
    /* Mark packet for hardware checksum */
    /* TODO: Set hardware descriptor flags */
    
    return 0;
}

int hw_offload_tcp_segmentation(sk_buff_t* skb, u32 mss) {
    if (!skb || mss == 0) {
        return -1;
    }
    
    hw_offload_device_t* dev = offload_devices;
    if (!dev || !(dev->enabled_features & OFFLOAD_CAP_TCP_TSO)) {
        return -1;
    }
    
    /* Hardware will segment large TCP packet */
    /* TODO: Setup TSO descriptor */
    
    return 0;
}

int hw_offload_rx_checksum(sk_buff_t* skb) {
    if (!skb) {
        return -1;
    }
    
    hw_offload_device_t* dev = offload_devices;
    if (!dev || !(dev->enabled_features & OFFLOAD_CAP_RX_CSUM)) {
        return -1;
    }
    
    /* Hardware calculated checksum */
    /* TODO: Read checksum result from descriptor */
    
    return 0;
}

int hw_offload_rx_hash(sk_buff_t* skb, u32* hash) {
    if (!skb || !hash) {
        return -1;
    }
    
    hw_offload_device_t* dev = offload_devices;
    if (!dev || !(dev->enabled_features & OFFLOAD_CAP_RX_HASH)) {
        return -1;
    }
    
    /* Hardware calculated hash */
    /* TODO: Read hash from descriptor */
    *hash = 0;
    
    return 0;
}

u32 hw_offload_get_capabilities(hw_offload_device_t* dev) {
    if (!dev) {
        return 0;
    }
    
    return dev->capabilities;
}

u32 hw_offload_get_enabled(hw_offload_device_t* dev) {
    if (!dev) {
        return 0;
    }
    
    return dev->enabled_features;
}

hw_offload_device_t* hw_offload_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&offload_lock);
    
    hw_offload_device_t* dev = offload_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&offload_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&offload_lock);
    return NULL;
}
