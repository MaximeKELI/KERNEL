#ifndef DEVICE_H
#define DEVICE_H

#include "types.h"

/* Device types */
#define DEV_CHAR  1
#define DEV_BLOCK 2

/* Device major/minor numbers */
#define DEV_MAJOR(dev) ((dev) >> 8)
#define DEV_MINOR(dev) ((dev) & 0xFF)
#define DEV_MAKE(major, minor) (((major) << 8) | (minor))

/* Device structure */
typedef struct device {
    u32 dev_id;
    u32 type;
    char name[64];
    void* private_data;
    struct device_ops* ops;
    struct device* next;
} device_t;

/* Device operations */
typedef struct device_ops {
    ssize_t (*read)(device_t* dev, void* buf, size_t count, u64 offset);
    ssize_t (*write)(device_t* dev, const void* buf, size_t count, u64 offset);
    int (*ioctl)(device_t* dev, u32 cmd, void* arg);
    int (*open)(device_t* dev);
    int (*close)(device_t* dev);
} device_ops_t;

/* Initialize device manager */
void device_init(void);

/* Register device */
int device_register(device_t* device);

/* Unregister device */
int device_unregister(u32 dev_id);

/* Find device */
device_t* device_find(u32 dev_id);

/* Get device by name */
device_t* device_find_by_name(const char* name);

/* Create device file */
int device_create(const char* path, u32 dev_id, u32 type);

#endif /* DEVICE_H */
