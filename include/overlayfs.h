#ifndef OVERLAYFS_H
#define OVERLAYFS_H

#include "types.h"
#include "fs/vfs.h"

/* Initialize overlayfs */
int overlayfs_init(void);

/* Mount overlayfs */
int overlayfs_mount(const char* lower, const char* upper, const char* work, const char* mountpoint);

/* Unmount overlayfs */
int overlayfs_unmount(const char* mountpoint);

#endif /* OVERLAYFS_H */
