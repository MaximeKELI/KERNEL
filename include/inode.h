#ifndef INODE_H
#define INODE_H

#include "types.h"

#include "refcount.h"

/* Inode structure */
typedef struct inode {
    refcount_t refcount;  /* Reference counting */
    u64 ino;              /* Inode number */
    u32 mode;             /* File mode */
    u32 nlink;            /* Number of hard links */
    u32 uid;              /* User ID */
    u32 gid;              /* Group ID */
    u64 size;             /* File size */
    u64 blocks;            /* Number of blocks */
    u64 atime;            /* Access time */
    u64 mtime;            /* Modification time */
    u64 ctime;            /* Change time */
    void* private_data;    /* Filesystem-specific data */
    struct inode_ops* ops; /* Inode operations */
} inode_t;

/* Inode operations */
typedef struct inode_ops {
    int (*lookup)(inode_t* dir, const char* name, inode_t** result);
    int (*create)(inode_t* dir, const char* name, u32 mode, inode_t** result);
    int (*unlink)(inode_t* dir, const char* name);
    int (*mkdir)(inode_t* dir, const char* name, u32 mode);
    int (*rmdir)(inode_t* dir, const char* name);
    ssize_t (*read)(inode_t* inode, void* buf, size_t count, u64 offset);
    ssize_t (*write)(inode_t* inode, const void* buf, size_t count, u64 offset);
    int (*readdir)(inode_t* dir, void* buf, size_t count, u64* offset);
} inode_ops_t;

/* Initialize inode system */
void inode_init(void);

/* Allocate inode */
inode_t* inode_alloc(void);

/* Free inode */
void inode_free(inode_t* inode);

/* Get inode by number */
inode_t* inode_get(u64 ino);

/* Put inode (decrement reference) */
void inode_put(inode_t* inode);

#endif /* INODE_H */
