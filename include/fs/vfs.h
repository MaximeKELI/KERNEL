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
    void* private_data;   /* owned by the backing filesystem's open() */
    int backend;          /* vfs_backend_type_t, set by vfs_open (see vfs.c) */
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

/* Poll: EPOLLIN if fd is open and readable */
int vfs_fd_poll_events(int fd);

/*
 * Per-process file-descriptor table (files_struct). Kept opaque (void*) so it
 * can hang off process_t.files without pulling VFS internals into process.h.
 *   files_create : fresh empty table
 *   files_clone  : fork() — share the same open files (refcounted) with the child
 *   files_on_exec: close descriptors marked close-on-exec
 *   files_destroy: exit() — drop every descriptor
 */
void* files_create(void);
void* files_clone(void* src);
void  files_on_exec(void* files);
void  files_destroy(void* files);

/* dup2-style: point new_fd at old_fd's open file within the current table. */
int vfs_dup2_fd(int old_fd, int new_fd);
int vfs_dup_fd(int old_fd);                 /* lowest free fd -> old_fd's file */
int vfs_set_cloexec(int fd, int on);

/* Install an already-built file (e.g. a pipe end) into the fd table. */
int vfs_install_file(vfs_file_t* file, struct vfs_ops* ops);

/* Directory / metadata operations (dispatch to the path's backend). */
int vfs_mkdir(const char* path, u32 mode);
int vfs_unlink(const char* path);
int vfs_rmdir(const char* path);

/* Minimal stat. */
typedef struct vfs_stat {
    u64 st_ino;
    u32 st_mode;
    u64 st_size;
} vfs_stat_t;
int vfs_stat(const char* path, vfs_stat_t* out);

/* getdents: fill up to `max` names starting at *pos (opaque index). */
typedef struct vfs_dent {
    u64 ino;
    u32 mode;
    char name[256];
} vfs_dent_t;
int vfs_getdents(const char* path, u32* pos, vfs_dent_t* out, int max);

/* fd-based getdents: uses the open directory fd's offset as the cursor. */
int vfs_getdents_fd(int fd, vfs_dent_t* out, int max);

/* Truncate the file behind an open fd (ramfs regular files). */
int vfs_ftruncate_fd(int fd, u64 length);

/* lseek on an open fd; returns new absolute offset or -1. */
ssize_t vfs_lseek_fd(int fd, ssize_t offset, int whence);

#endif /* VFS_H */
