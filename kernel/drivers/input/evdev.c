#include "evdev.h"
#include "pci.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "memory.h"
#include "types.h"
#include "string.h"

#define MAX_EVDEV_DEVICES 16

/* Input Event Types */
#define EV_SYN      0x00
#define EV_KEY      0x01
#define EV_REL      0x02
#define EV_ABS      0x03
#define EV_MSC      0x04

/* Input Event */
typedef struct input_event {
    u64 timestamp;
    u16 type;
    u16 code;
    i32 value;
} input_event_t;

/* Evdev Device */
typedef struct evdev_device {
    u32 id;
    char name[64];
    u32 vendor_id;
    u32 product_id;
    u32 version;
    u32 event_types;
    bool keyboard;
    bool mouse;
    bool touchpad;
    void* buffer;
    u32 buffer_size;
    u32 buffer_head;
    u32 buffer_tail;
    bool initialized;
    struct evdev_device* next;
} evdev_device_t;

static evdev_device_t* evdev_devices = NULL;
static u32 evdev_device_count = 0;
static spinlock_t evdev_lock = SPINLOCK_INIT;
static bool evdev_initialized = false;

void evdev_init(void) {
    if (evdev_initialized) {
        return;
    }
    
    evdev_devices = NULL;
    evdev_device_count = 0;
    evdev_initialized = true;
    
    /* Scan PCI for input controllers */
    pci_device_t* pci_dev = pci_find_class(0x0C, 0x03); /* USB Input Device */
    if (pci_dev) {
        DEBUG_INFO("Input controller found: %04x:%04x", 
                  pci_dev->vendor_id, pci_dev->device_id);
        
        evdev_device_t* dev = evdev_alloc_device();
        if (dev) {
            dev->vendor_id = pci_dev->vendor_id;
            dev->product_id = pci_dev->device_id;
            dev->keyboard = true;
            dev->mouse = false;
            dev->touchpad = false;
            
            snprintf(dev->name, sizeof(dev->name), "input%d", dev->id);
            
            DEBUG_INFO("Evdev device created: %s", dev->name);
        }
    }
    
    printk("[Evdev] Input device subsystem initialized\n");
}

evdev_device_t* evdev_alloc_device(void) {
    if (evdev_device_count >= MAX_EVDEV_DEVICES) {
        return NULL;
    }
    
    evdev_device_t* dev = (evdev_device_t*)kzalloc(sizeof(evdev_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->id = evdev_device_count++;
    dev->initialized = false;
    dev->buffer_size = 1024; /* 1024 events */
    dev->buffer = kzalloc(dev->buffer_size * sizeof(input_event_t));
    dev->buffer_head = 0;
    dev->buffer_tail = 0;
    
    if (!dev->buffer) {
        kfree(dev);
        return NULL;
    }
    
    spinlock_lock(&evdev_lock);
    dev->next = evdev_devices;
    evdev_devices = dev;
    spinlock_unlock(&evdev_lock);
    
    return dev;
}

void evdev_free_device(evdev_device_t* dev) {
    if (!dev) {
        return;
    }
    
    if (dev->buffer) {
        kfree(dev->buffer);
    }
    
    spinlock_lock(&evdev_lock);
    
    if (evdev_devices == dev) {
        evdev_devices = dev->next;
    } else {
        evdev_device_t* p = evdev_devices;
        while (p && p->next != dev) {
            p = p->next;
        }
        if (p) {
            p->next = dev->next;
        }
    }
    
    evdev_device_count--;
    spinlock_unlock(&evdev_lock);
    
    kfree(dev);
}

int evdev_write_event(evdev_device_t* dev, input_event_t* event) {
    if (!dev || !dev->initialized || !event) {
        return -1;
    }
    
    spinlock_lock(&evdev_lock);
    
    u32 next = (dev->buffer_tail + 1) % dev->buffer_size;
    if (next == dev->buffer_head) {
        /* Buffer full */
        spinlock_unlock(&evdev_lock);
        return -1;
    }
    
    input_event_t* buf = (input_event_t*)dev->buffer;
    buf[dev->buffer_tail] = *event;
    dev->buffer_tail = next;
    
    spinlock_unlock(&evdev_lock);
    
    return 0;
}

int evdev_read_event(evdev_device_t* dev, input_event_t* event) {
    if (!dev || !dev->initialized || !event) {
        return -1;
    }
    
    spinlock_lock(&evdev_lock);
    
    if (dev->buffer_head == dev->buffer_tail) {
        /* Buffer empty */
        spinlock_unlock(&evdev_lock);
        return 0;
    }
    
    input_event_t* buf = (input_event_t*)dev->buffer;
    *event = buf[dev->buffer_head];
    dev->buffer_head = (dev->buffer_head + 1) % dev->buffer_size;
    
    spinlock_unlock(&evdev_lock);
    
    return 1;
}

evdev_device_t* evdev_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&evdev_lock);
    
    evdev_device_t* dev = evdev_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&evdev_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&evdev_lock);
    return NULL;
}

u32 evdev_get_device_count(void) {
    spinlock_lock(&evdev_lock);
    u32 count = evdev_device_count;
    spinlock_unlock(&evdev_lock);
    return count;
}
