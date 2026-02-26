#ifndef FUSE_H
#define FUSE_H

#include "types.h"
#include "fs/vfs.h"

/* FUSE operations */
typedef struct fuse_operations {
    int (*getattr)(const char* path, void* stbuf);
    int (*readdir)(const char* path, void* buf, void* filler, u64 offset);
    int (*open)(const char* path, vfs_file_t* file);
    int (*read)(const char* path, char* buf, size_t size, u64 offset, vfs_file_t* file);
    int (*write)(const char* path, const char* buf, size_t size, u64 offset, vfs_file_t* file);
    int (*mkdir)(const char* path, u32 mode);
    int (*unlink)(const char* path);
} fuse_operations_t;

/* Initialize FUSE */
void fuse_init(void);

/* Mount FUSE filesystem */
int fuse_mount(const char* mountpoint, fuse_operations_t* ops);

/* Unmount FUSE filesystem */
int fuse_unmount(const char* mountpoint);

#endif /* FUSE_H */
