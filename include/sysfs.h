#ifndef SYSFS_H
#define SYSFS_H

#include "types.h"
#include "fs/vfs.h"

/* Initialize sysfs */
int sysfs_init(void);

/* Mount sysfs */
int sysfs_mount(const char* mountpoint);

/* Create sysfs entry */
typedef struct sysfs_entry {
    char name[64];
    vfs_file_t* file;
    struct sysfs_entry* parent;
    struct sysfs_entry* children;
    struct sysfs_entry* sibling;
} sysfs_entry_t;

/* Create sysfs file */
int sysfs_create_file(const char* path, vfs_file_t* file);

/* Remove sysfs file */
int sysfs_remove_file(const char* path);

/* Read sysfs attribute */
ssize_t sysfs_read_attr(const char* path, void* buf, size_t count);

/* Write sysfs attribute */
ssize_t sysfs_write_attr(const char* path, const void* buf, size_t count);

#endif /* SYSFS_H */
