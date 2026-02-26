#ifndef XFS_H
#define XFS_H

#include "types.h"
#include "fs/vfs.h"

/* XFS features */
#define XFS_FEATURE_REALTIME    0x01
#define XFS_FEATURE_EXTENTS    0x02
#define XFS_FEATURE_BTREE      0x04
#define XFS_FEATURE_QUOTA      0x08

/* XFS allocation groups */
typedef struct xfs_ag {
    u32 ag_number;
    u64 ag_size;
    u64 free_blocks;
    struct xfs_ag* next;
} xfs_ag_t;

/* Initialize XFS */
int xfs_init(void);

/* Mount XFS */
int xfs_mount(const char* source, const char* target);

/* Unmount XFS */
int xfs_unmount(const char* target);

/* Get filesystem statistics */
int xfs_statfs(const char* path, void* buf);

/* Defragment file */
int xfs_defrag(const char* path);

#endif /* XFS_H */
