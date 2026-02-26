#include "device.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "fs/vfs.h"

#define MAX_DEVICES 256

static device_t* device_list = NULL;
static spinlock_t device_lock = SPINLOCK_INIT;

void device_init(void) {
    DEBUG_INFO("Device manager initialized");
}

int device_register(device_t* device) {
    if (!device) {
        DEBUG_ERROR("Attempted to register NULL device");
        return -1;
    }
    
    spinlock_lock(&device_lock);
    
    /* Check if already registered */
    device_t* existing = device_list;
    while (existing) {
        if (existing->dev_id == device->dev_id) {
            DEBUG_WARN("Device %u already registered", device->dev_id);
            spinlock_unlock(&device_lock);
            return -1;
        }
        existing = existing->next;
    }
    
    device->next = device_list;
    device_list = device;
    
    spinlock_unlock(&device_lock);
    
    DEBUG_INFO("Device registered: %s (id: %u, type: %u)",
               device->name, device->dev_id, device->type);
    
    return 0;
}

int device_unregister(u32 dev_id) {
    spinlock_lock(&device_lock);
    
    device_t** prev = &device_list;
    device_t* dev = device_list;
    
    while (dev) {
        if (dev->dev_id == dev_id) {
            *prev = dev->next;
            spinlock_unlock(&device_lock);
            DEBUG_INFO("Device unregistered: %u", dev_id);
            return 0;
        }
        prev = &dev->next;
        dev = dev->next;
    }
    
    spinlock_unlock(&device_lock);
    DEBUG_WARN("Device %u not found for unregister", dev_id);
    return -1;
}

device_t* device_find(u32 dev_id) {
    spinlock_lock(&device_lock);
    
    device_t* dev = device_list;
    while (dev) {
        if (dev->dev_id == dev_id) {
            spinlock_unlock(&device_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&device_lock);
    return NULL;
}

device_t* device_find_by_name(const char* name) {
    if (!name) return NULL;
    
    spinlock_lock(&device_lock);
    
    device_t* dev = device_list;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&device_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&device_lock);
    return NULL;
}

int device_create(const char* path, u32 dev_id, u32 type) {
    (void)path;
    (void)dev_id;
    (void)type;
    /* Would create device file in /dev */
    return 0;
}
