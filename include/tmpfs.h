#ifndef TMPFS_H
#define TMPFS_H

#include "types.h"
#include "fs/vfs.h"

/* Initialize tmpfs */
int tmpfs_init(void);

/* Mount tmpfs */
int tmpfs_mount(const char* mountpoint, size_t size);

/* Get filesystem operations */
vfs_fs_ops_t* tmpfs_get_fs_ops(void);

#endif /* TMPFS_H */
