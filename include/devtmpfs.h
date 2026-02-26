#ifndef DEVTMPFS_H
#define DEVTMPFS_H

#include "types.h"
#include "fs/vfs.h"

/* Initialize devtmpfs */
int devtmpfs_init(void);

/* Mount devtmpfs */
int devtmpfs_mount(const char* mountpoint);

/* Create device node */
int devtmpfs_create_device(const char* path, u32 major, u32 minor, u32 mode);

/* Remove device node */
int devtmpfs_remove_device(const char* path);

/* Handle hotplug */
int devtmpfs_hotplug_add(const char* path, u32 major, u32 minor);
int devtmpfs_hotplug_remove(const char* path);

#endif /* DEVTMPFS_H */
