#include "drm.h"
#include "pci.h"
#include "framebuffer.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "memory.h"
#include "types.h"
#include "string.h"

#define MAX_DRM_DEVICES 4

/* DRM Device */
typedef struct drm_device {
    u32 id;
    char name[32];
    u32 vendor_id;
    u32 device_id;
    void* mmio_base;
    u64 mmio_size;
    u32 width;
    u32 height;
    u32 bpp;
    void* framebuffer;
    u64 framebuffer_size;
    bool initialized;
    struct drm_device* next;
} drm_device_t;

static drm_device_t* drm_devices = NULL;
static u32 drm_device_count = 0;
static spinlock_t drm_lock = SPINLOCK_INIT;
static bool drm_initialized = false;

void drm_init(void) {
    if (drm_initialized) {
        return;
    }
    
    drm_devices = NULL;
    drm_device_count = 0;
    drm_initialized = true;
    
    /* Scan PCI for graphics controllers */
    pci_device_t* pci_dev = pci_find_class(0x03, 0x00); /* VGA Compatible Controller */
    if (!pci_dev) {
        pci_dev = pci_find_class(0x03, 0x80); /* Other Display Controller */
    }
    
    if (pci_dev) {
        DEBUG_INFO("Graphics controller found: %04x:%04x", 
                  pci_dev->vendor_id, pci_dev->device_id);
        
        drm_device_t* dev = drm_alloc_device();
        if (dev) {
            dev->vendor_id = pci_dev->vendor_id;
            dev->device_id = pci_dev->device_id;
            dev->mmio_base = (void*)(pci_dev->bar[0] & ~0xF);
            dev->mmio_size = 0x1000000; /* 16MB */
            dev->width = 1920;
            dev->height = 1080;
            dev->bpp = 32;
            
            snprintf(dev->name, sizeof(dev->name), "card%d", dev->id);
            
            DEBUG_INFO("DRM device created: %s (%ux%u@%ubpp)", 
                      dev->name, dev->width, dev->height, dev->bpp);
        }
    }
    
    printk("[DRM] Direct Rendering Manager initialized\n");
}

drm_device_t* drm_alloc_device(void) {
    if (drm_device_count >= MAX_DRM_DEVICES) {
        return NULL;
    }
    
    drm_device_t* dev = (drm_device_t*)kzalloc(sizeof(drm_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->id = drm_device_count++;
    dev->initialized = false;
    
    spinlock_lock(&drm_lock);
    dev->next = drm_devices;
    drm_devices = dev;
    spinlock_unlock(&drm_lock);
    
    return dev;
}

void drm_free_device(drm_device_t* dev) {
    if (!dev) {
        return;
    }
    
    if (dev->framebuffer) {
        vmm_free_pages(dev->framebuffer, 
                      (dev->framebuffer_size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
    
    spinlock_lock(&drm_lock);
    
    if (drm_devices == dev) {
        drm_devices = dev->next;
    } else {
        drm_device_t* p = drm_devices;
        while (p && p->next != dev) {
            p = p->next;
        }
        if (p) {
            p->next = dev->next;
        }
    }
    
    drm_device_count--;
    spinlock_unlock(&drm_lock);
    
    kfree(dev);
}

int drm_set_mode(drm_device_t* dev, u32 width, u32 height, u32 bpp) {
    if (!dev) {
        return -1;
    }
    
    dev->width = width;
    dev->height = height;
    dev->bpp = bpp;
    
    /* Allocate framebuffer */
    if (dev->framebuffer) {
        vmm_free_pages(dev->framebuffer, 
                      (dev->framebuffer_size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
    
    dev->framebuffer_size = width * height * (bpp / 8);
    dev->framebuffer = vmm_alloc_pages((dev->framebuffer_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!dev->framebuffer) {
        DEBUG_ERROR("Failed to allocate framebuffer");
        return -1;
    }
    
    dev->initialized = true;
    
    DEBUG_INFO("DRM mode set: %ux%u@%ubpp", width, height, bpp);
    return 0;
}

void* drm_get_framebuffer(drm_device_t* dev) {
    if (!dev || !dev->initialized) {
        return NULL;
    }
    
    return dev->framebuffer;
}

drm_device_t* drm_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&drm_lock);
    
    drm_device_t* dev = drm_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&drm_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&drm_lock);
    return NULL;
}

u32 drm_get_device_count(void) {
    spinlock_lock(&drm_lock);
    u32 count = drm_device_count;
    spinlock_unlock(&drm_lock);
    return count;
}
