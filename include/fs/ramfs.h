#ifndef RAMFS_H
#define RAMFS_H

#include "types.h"
#include <stddef.h>

/*
 * ramfs: a real in-memory hierarchical filesystem (inode + dentry tree). Unlike
 * the old flat, path-keyed tmpfs, this supports directories, path resolution
 * (including "." / ".."), create/mkdir/unlink/rmdir, readdir and growing files.
 * It backs the VFS root so the file syscalls have a genuine tree to operate on.
 */

#define RAMFS_NAME_MAX 60

/* Mode type bits (match the ext2/Linux S_IF* layout). */
#define RAMFS_IFDIR 0x4000
#define RAMFS_IFREG 0x8000

typedef struct ramfs_node {
    char name[RAMFS_NAME_MAX + 1];
    u32 mode;                 /* RAMFS_IFDIR | RAMFS_IFREG */
    u64 ino;
    u64 size;
    u8* data;                 /* file contents (regular files) */
    u64 capacity;
    u32 nlink;
    struct ramfs_node* parent;
    struct ramfs_node* children;   /* first child (directories) */
    struct ramfs_node* sibling;    /* next entry in parent's list */
} ramfs_node_t;

void ramfs_init(void);
ramfs_node_t* ramfs_root(void);

/* Resolve an absolute path to a node, or NULL. */
ramfs_node_t* ramfs_resolve(const char* path);

/* Create a regular file (parent dir must exist). Returns node or NULL. */
ramfs_node_t* ramfs_create(const char* path, u32 mode);

/* Create a directory. Returns node or NULL (already-exists -> NULL). */
ramfs_node_t* ramfs_mkdir(const char* path);

int ramfs_unlink(const char* path);   /* remove a regular file */
int ramfs_rmdir(const char* path);    /* remove an empty directory */

ssize_t ramfs_read(ramfs_node_t* n, u64 offset, void* buf, size_t count);
ssize_t ramfs_write(ramfs_node_t* n, u64 offset, const void* buf, size_t count);
int ramfs_truncate(ramfs_node_t* n, u64 size);

/*
 * Enumerate directory entries by index (0-based). Fills name/ino/mode for entry
 * `index` (including "." at 0 and ".." at 1). Returns 1 on success, 0 at end.
 */
int ramfs_readdir_index(ramfs_node_t* dir, u32 index,
                        char* name_out, u64* ino_out, u32* mode_out);

#endif /* RAMFS_H */
