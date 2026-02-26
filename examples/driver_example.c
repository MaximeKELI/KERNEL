/*
 * Example Device Driver
 * 
 * This is a template for creating new device drivers in the kernel.
 */

#include "device.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

/* Driver structure */
typedef struct example_driver {
    device_t* device;
    void* private_data;
    spinlock_t lock;
    bool initialized;
} example_driver_t;

static example_driver_t driver_instance = {0};

/* Device operations */
static int example_open(device_t* dev) {
    VALIDATE_PTR_RET(dev, -1);
    
    DEBUG_INFO("Example driver: device opened");
    return 0;
}

static int example_close(device_t* dev) {
    VALIDATE_PTR_RET(dev, -1);
    
    DEBUG_INFO("Example driver: device closed");
    return 0;
}

static ssize_t example_read(device_t* dev, void* buf, size_t count) {
    VALIDATE_PTR_RET(dev, -1);
    VALIDATE_PTR_RET(buf, -1);
    
    /* Read from device */
    memset(buf, 0, count);
    DEBUG_INFO("Example driver: read %u bytes", (u32)count);
    return (ssize_t)count;
}

static ssize_t example_write(device_t* dev, const void* buf, size_t count) {
    VALIDATE_PTR_RET(dev, -1);
    VALIDATE_PTR_RET(buf, -1);
    
    /* Write to device */
    DEBUG_INFO("Example driver: wrote %u bytes", (u32)count);
    return (ssize_t)count;
}

/* Driver operations */
static device_ops_t example_ops = {
    .open = example_open,
    .close = example_close,
    .read = example_read,
    .write = example_write,
    .ioctl = NULL,
    .mmap = NULL
};

/* Initialize driver */
int example_driver_init(void) {
    if (driver_instance.initialized) {
        return 0;
    }
    
    spinlock_init(&driver_instance.lock);
    
    /* Register device */
    driver_instance.device = device_register("example", DEVICE_TYPE_CHAR, &example_ops);
    if (!driver_instance.device) {
        DEBUG_ERROR("Failed to register example device");
        return -1;
    }
    
    driver_instance.initialized = true;
    DEBUG_INFO("Example driver initialized");
    return 0;
}

/* Cleanup driver */
void example_driver_cleanup(void) {
    if (!driver_instance.initialized) {
        return;
    }
    
    if (driver_instance.device) {
        device_unregister(driver_instance.device);
    }
    
    driver_instance.initialized = false;
    DEBUG_INFO("Example driver cleaned up");
}
