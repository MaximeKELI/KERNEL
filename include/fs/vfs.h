#ifndef VFS_H
#define VFS_H

#include "types.h"
#include <stddef.h>

/* ssize_t for signed size */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

/* File modes */
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040
#define O_TRUNC  0x0200
#define O_APPEND 0x0400

/* File types */
typedef enum {
    VFS_FILE,
    VFS_DIRECTORY,
    VFS_CHAR_DEVICE,
    VFS_BLOCK_DEVICE
} vfs_type_t;

/* File structure */
typedef struct vfs_file {
    u64 inode;
    vfs_type_t type;
    size_t size;
    u64 flags;
    u64 offset;
    void* private_data;
} vfs_file_t;

/* Directory entry */
typedef struct vfs_dirent {
    char name[256];
    u64 inode;
    vfs_type_t type;
} vfs_dirent_t;

/* File operations */
typedef struct vfs_ops {
    int (*open)(vfs_file_t* file, const char* path);
    int (*close)(vfs_file_t* file);
    ssize_t (*read)(vfs_file_t* file, void* buf, size_t count);
    ssize_t (*write)(vfs_file_t* file, const void* buf, size_t count);
    int (*seek)(vfs_file_t* file, ssize_t offset, int whence);
    int (*readdir)(vfs_file_t* file, vfs_dirent_t* dirent);
} vfs_ops_t;

/* Filesystem operations */
typedef struct vfs_fs_ops {
    int (*mount)(const char* source, const char* target);
    int (*unmount)(const char* target);
    vfs_ops_t* (*get_ops)(void);
} vfs_fs_ops_t;

/* Initialize VFS */
void vfs_init(void);

/* Mount filesystem */
int vfs_mount(const char* source, const char* target, vfs_fs_ops_t* fs_ops);

/* Unmount filesystem */
int vfs_unmount(const char* target);

/* Open file */
vfs_file_t* vfs_open(const char* path, u64 flags);

/* Close file */
int vfs_close(vfs_file_t* file);

/* Read from file */
ssize_t vfs_read(vfs_file_t* file, void* buf, size_t count);

/* Write to file */
ssize_t vfs_write(vfs_file_t* file, const void* buf, size_t count);

/* Register filesystem */
void vfs_register_filesystem(const char* name, vfs_fs_ops_t* fs_ops);

/* FD-based API (POSIX-like) */
int vfs_open_fd(const char* path, u64 flags);
int vfs_close_fd(int fd);
ssize_t vfs_read_fd(int fd, void* buf, size_t count);
ssize_t vfs_write_fd(int fd, const void* buf, size_t count);
ssize_t vfs_read_path(const char* path, void* buf, size_t count);
ssize_t vfs_write_path(const char* path, const void* buf, size_t count);

#endif /* VFS_H */
