#include "alsa.h"
#include "pci.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "memory.h"
#include "types.h"
#include "string.h"

#define MAX_ALSA_DEVICES 4
#define ALSA_SAMPLE_RATE 44100
#define ALSA_CHANNELS 2
#define ALSA_BITS_PER_SAMPLE 16

/* ALSA Device */
typedef struct alsa_device {
    u32 id;
    char name[32];
    u32 vendor_id;
    u32 device_id;
    void* mmio_base;
    u32 sample_rate;
    u32 channels;
    u32 bits_per_sample;
    void* buffer;
    u64 buffer_size;
    bool initialized;
    struct alsa_device* next;
} alsa_device_t;

static alsa_device_t* alsa_devices = NULL;
static u32 alsa_device_count = 0;
static spinlock_t alsa_lock = SPINLOCK_INIT;
static bool alsa_initialized = false;

void alsa_init(void) {
    if (alsa_initialized) {
        return;
    }
    
    alsa_devices = NULL;
    alsa_device_count = 0;
    alsa_initialized = true;
    
    /* Scan PCI for audio controllers */
    pci_device_t* pci_dev = pci_find_class(0x04, 0x01); /* Audio Device */
    if (pci_dev) {
        DEBUG_INFO("Audio controller found: %04x:%04x", 
                  pci_dev->vendor_id, pci_dev->device_id);
        
        alsa_device_t* dev = alsa_alloc_device();
        if (dev) {
            dev->vendor_id = pci_dev->vendor_id;
            dev->device_id = pci_dev->device_id;
            dev->mmio_base = (void*)(pci_dev->bar[0] & ~0xF);
            dev->sample_rate = ALSA_SAMPLE_RATE;
            dev->channels = ALSA_CHANNELS;
            dev->bits_per_sample = ALSA_BITS_PER_SAMPLE;
            
            snprintf(dev->name, sizeof(dev->name), "card%d", dev->id);
            
            DEBUG_INFO("ALSA device created: %s", dev->name);
        }
    }
    
    printk("[ALSA] Advanced Linux Sound Architecture initialized\n");
}

alsa_device_t* alsa_alloc_device(void) {
    if (alsa_device_count >= MAX_ALSA_DEVICES) {
        return NULL;
    }
    
    alsa_device_t* dev = (alsa_device_t*)kzalloc(sizeof(alsa_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->id = alsa_device_count++;
    dev->initialized = false;
    dev->sample_rate = ALSA_SAMPLE_RATE;
    dev->channels = ALSA_CHANNELS;
    dev->bits_per_sample = ALSA_BITS_PER_SAMPLE;
    
    spinlock_lock(&alsa_lock);
    dev->next = alsa_devices;
    alsa_devices = dev;
    spinlock_unlock(&alsa_lock);
    
    return dev;
}

void alsa_free_device(alsa_device_t* dev) {
    if (!dev) {
        return;
    }
    
    if (dev->buffer) {
        vmm_free_pages(dev->buffer, 
                      (dev->buffer_size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
    
    spinlock_lock(&alsa_lock);
    
    if (alsa_devices == dev) {
        alsa_devices = dev->next;
    } else {
        alsa_device_t* p = alsa_devices;
        while (p && p->next != dev) {
            p = p->next;
        }
        if (p) {
            p->next = dev->next;
        }
    }
    
    alsa_device_count--;
    spinlock_unlock(&alsa_lock);
    
    kfree(dev);
}

int alsa_set_params(alsa_device_t* dev, u32 sample_rate, u32 channels, u32 bits_per_sample) {
    if (!dev) {
        return -1;
    }
    
    dev->sample_rate = sample_rate;
    dev->channels = channels;
    dev->bits_per_sample = bits_per_sample;
    
    /* Allocate audio buffer */
    if (dev->buffer) {
        vmm_free_pages(dev->buffer, 
                      (dev->buffer_size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
    
    dev->buffer_size = sample_rate * channels * (bits_per_sample / 8) * 2; /* 2 seconds */
    dev->buffer = vmm_alloc_pages((dev->buffer_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!dev->buffer) {
        DEBUG_ERROR("Failed to allocate audio buffer");
        return -1;
    }
    
    dev->initialized = true;
    
    DEBUG_INFO("ALSA params set: %u Hz, %u channels, %u bits", 
              sample_rate, channels, bits_per_sample);
    return 0;
}

int alsa_write(alsa_device_t* dev, void* data, size_t len) {
    if (!dev || !dev->initialized || !data || len == 0) {
        return -1;
    }
    
    /* TODO: Write audio data to hardware */
    return 0;
}

int alsa_read(alsa_device_t* dev, void* buffer, size_t len) {
    if (!dev || !dev->initialized || !buffer || len == 0) {
        return -1;
    }
    
    /* TODO: Read audio data from hardware */
    return 0;
}

alsa_device_t* alsa_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&alsa_lock);
    
    alsa_device_t* dev = alsa_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&alsa_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&alsa_lock);
    return NULL;
}

u32 alsa_get_device_count(void) {
    spinlock_lock(&alsa_lock);
    u32 count = alsa_device_count;
    spinlock_unlock(&alsa_lock);
    return count;
}
