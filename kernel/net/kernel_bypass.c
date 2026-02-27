#include "kernel_bypass.h"
#include "memory.h"
#include "ethernet.h"
#include "skbuff.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "io.h"

#define MAX_BYPASS_QUEUES 64
#define BYPASS_RING_SIZE 4096
#define BYPASS_DESC_SIZE 16

/* Bypass queue descriptor */
typedef struct __packed {
    u64 addr;           /* Physical address */
    u32 length;         /* Buffer length */
    u16 flags;          /* Descriptor flags */
    u16 vlan;           /* VLAN tag */
} bypass_desc_t;

/* Bypass queue */
typedef struct bypass_queue {
    u32 queue_id;
    u32 ring_size;
    bypass_desc_t* rx_ring;
    bypass_desc_t* tx_ring;
    u32 rx_head;
    u32 rx_tail;
    u32 tx_head;
    u32 tx_tail;
    void* rx_buffers[BYPASS_RING_SIZE];
    void* tx_buffers[BYPASS_RING_SIZE];
    u64 rx_packets;
    u64 tx_packets;
    u64 rx_bytes;
    u64 tx_bytes;
    bool enabled;
    spinlock_t lock;
    struct bypass_queue* next;
} bypass_queue_t;

/* Bypass device */
typedef struct bypass_device {
    u32 device_id;
    char name[32];
    ethernet_device_t* eth_dev;
    bypass_queue_t* queues[MAX_BYPASS_QUEUES];
    u32 num_queues;
    u64 rx_dropped;
    u64 tx_dropped;
    bool enabled;
    struct bypass_device* next;
} bypass_device_t;

static bypass_device_t* bypass_devices = NULL;
static spinlock_t bypass_global_lock = SPINLOCK_INIT;

void kernel_bypass_init(void) {
    bypass_devices = NULL;
    printk("[Kernel Bypass] Kernel bypass networking initialized\n");
}

bypass_device_t* kernel_bypass_create_device(const char* name, ethernet_device_t* eth_dev, u32 num_queues) {
    if (!name || !eth_dev || num_queues == 0 || num_queues > MAX_BYPASS_QUEUES) {
        return NULL;
    }
    
    bypass_device_t* dev = (bypass_device_t*)kzalloc(sizeof(bypass_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->device_id = 0; /* TODO: Generate unique ID */
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->eth_dev = eth_dev;
    dev->num_queues = num_queues;
    dev->enabled = false;
    
    /* Allocate queues */
    for (u32 i = 0; i < num_queues; i++) {
        bypass_queue_t* queue = (bypass_queue_t*)kzalloc(sizeof(bypass_queue_t));
        if (!queue) {
            /* Cleanup */
            for (u32 j = 0; j < i; j++) {
                kfree(dev->queues[j]);
            }
            kfree(dev);
            return NULL;
        }
        
        queue->queue_id = i;
        queue->ring_size = BYPASS_RING_SIZE;
        queue->rx_head = 0;
        queue->rx_tail = 0;
        queue->tx_head = 0;
        queue->tx_tail = 0;
        queue->enabled = false;
        spinlock_init(&queue->lock);
        
        /* Allocate descriptor rings */
        size_t ring_bytes = BYPASS_RING_SIZE * sizeof(bypass_desc_t);
        queue->rx_ring = (bypass_desc_t*)vmm_alloc_pages((ring_bytes + PAGE_SIZE - 1) / PAGE_SIZE);
        queue->tx_ring = (bypass_desc_t*)vmm_alloc_pages((ring_bytes + PAGE_SIZE - 1) / PAGE_SIZE);
        
        if (!queue->rx_ring || !queue->tx_ring) {
            if (queue->rx_ring) vmm_free_pages(queue->rx_ring, (ring_bytes + PAGE_SIZE - 1) / PAGE_SIZE);
            if (queue->tx_ring) vmm_free_pages(queue->tx_ring, (ring_bytes + PAGE_SIZE - 1) / PAGE_SIZE);
            kfree(queue);
            for (u32 j = 0; j < i; j++) {
                kfree(dev->queues[j]);
            }
            kfree(dev);
            return NULL;
        }
        
        /* Allocate buffer pools */
        for (u32 j = 0; j < BYPASS_RING_SIZE; j++) {
            queue->rx_buffers[j] = vmm_alloc_pages(1); /* 4KB buffer */
            queue->tx_buffers[j] = vmm_alloc_pages(1);
        }
        
        dev->queues[i] = queue;
    }
    
    spinlock_lock(&bypass_global_lock);
    dev->next = bypass_devices;
    bypass_devices = dev;
    spinlock_unlock(&bypass_global_lock);
    
    DEBUG_INFO("Kernel bypass device created: %s, %u queues", name, num_queues);
    return dev;
}

int kernel_bypass_enable(bypass_device_t* dev) {
    if (!dev) {
        return -1;
    }
    
    dev->enabled = true;
    
    for (u32 i = 0; i < dev->num_queues; i++) {
        bypass_queue_t* queue = dev->queues[i];
        queue->enabled = true;
        
        /* Setup hardware rings */
        /* TODO: Program NIC registers for bypass mode */
    }
    
    DEBUG_INFO("Kernel bypass enabled for device: %s", dev->name);
    return 0;
}

int kernel_bypass_disable(bypass_device_t* dev) {
    if (!dev) {
        return -1;
    }
    
    dev->enabled = false;
    
    for (u32 i = 0; i < dev->num_queues; i++) {
        dev->queues[i]->enabled = false;
    }
    
    return 0;
}

u32 kernel_bypass_rx_burst(bypass_device_t* dev, u32 queue_id, 
                           void** packets, u32 max_packets) {
    if (!dev || !dev->enabled || queue_id >= dev->num_queues) {
        return 0;
    }
    
    bypass_queue_t* queue = dev->queues[queue_id];
    if (!queue->enabled) {
        return 0;
    }
    
    spinlock_lock(&queue->lock);
    
    u32 received = 0;
    u32 tail = queue->rx_tail;
    
    while (received < max_packets) {
        u32 next = (tail + 1) % queue->ring_size;
        
        if (next == queue->rx_head) {
            break; /* Ring empty */
        }
        
        bypass_desc_t* desc = &queue->rx_ring[tail];
        
        /* Check if descriptor is ready */
        if (!(desc->flags & 0x01)) { /* Ready flag */
            break;
        }
        
        /* Return buffer to caller */
        packets[received] = queue->rx_buffers[tail];
        
        /* Update statistics */
        queue->rx_packets++;
        queue->rx_bytes += desc->length;
        
        /* Clear descriptor */
        desc->flags = 0;
        desc->length = 0;
        
        tail = next;
        received++;
    }
    
    queue->rx_tail = tail;
    
    spinlock_unlock(&queue->lock);
    
    return received;
}

u32 kernel_bypass_tx_burst(bypass_device_t* dev, u32 queue_id,
                           void** packets, u32* lengths, u32 num_packets) {
    if (!dev || !dev->enabled || queue_id >= dev->num_queues) {
        return 0;
    }
    
    bypass_queue_t* queue = dev->queues[queue_id];
    if (!queue->enabled) {
        return 0;
    }
    
    spinlock_lock(&queue->lock);
    
    u32 sent = 0;
    u32 head = queue->tx_head;
    
    while (sent < num_packets) {
        u32 next = (head + 1) % queue->ring_size;
        
        if (next == queue->tx_tail) {
            /* Ring full */
            dev->tx_dropped++;
            break;
        }
        
        /* Get physical address of packet */
        void* virt = packets[sent];
        void* phys = vmm_get_phys(virt);
        
        if (!phys) {
            sent++;
            continue;
        }
        
        /* Setup descriptor */
        bypass_desc_t* desc = &queue->tx_ring[head];
        desc->addr = (u64)phys;
        desc->length = lengths[sent];
        desc->flags = 0x01; /* Ready to send */
        
        /* Update statistics */
        queue->tx_packets++;
        queue->tx_bytes += lengths[sent];
        
        head = next;
        sent++;
    }
    
    queue->tx_head = head;
    
    /* Notify hardware */
    /* TODO: Write to NIC doorbell register */
    
    spinlock_unlock(&queue->lock);
    
    return sent;
}

void kernel_bypass_get_stats(bypass_device_t* dev, u32 queue_id,
                             u64* rx_packets, u64* tx_packets,
                             u64* rx_bytes, u64* tx_bytes) {
    if (!dev || queue_id >= dev->num_queues) {
        return;
    }
    
    bypass_queue_t* queue = dev->queues[queue_id];
    
    spinlock_lock(&queue->lock);
    if (rx_packets) *rx_packets = queue->rx_packets;
    if (tx_packets) *tx_packets = queue->tx_packets;
    if (rx_bytes) *rx_bytes = queue->rx_bytes;
    if (tx_bytes) *tx_bytes = queue->tx_bytes;
    spinlock_unlock(&queue->lock);
}

bypass_device_t* kernel_bypass_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&bypass_global_lock);
    
    bypass_device_t* dev = bypass_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&bypass_global_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&bypass_global_lock);
    return NULL;
}
