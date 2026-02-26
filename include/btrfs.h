#ifndef BTRFS_H
#define BTRFS_H

#include "types.h"
#include "fs/vfs.h"

/* Btrfs features */
#define BTRFS_FEATURE_COMPRESSION 0x01
#define BTRFS_FEATURE_SNAPSHOTS   0x02
#define BTRFS_FEATURE_SUBVOLUMES  0x04
#define BTRFS_FEATURE_RAID        0x08

/* Btrfs subvolume structure */
typedef struct btrfs_subvolume {
    u64 id;
    char name[256];
    u64 parent_id;
    struct btrfs_subvolume* next;
} btrfs_subvolume_t;

/* Initialize Btrfs */
int btrfs_init(void);

/* Mount Btrfs */
int btrfs_mount(const char* source, const char* target);

/* Unmount Btrfs */
int btrfs_unmount(const char* target);

/* Create subvolume */
int btrfs_create_subvolume(const char* path, const char* name);

/* Delete subvolume */
int btrfs_delete_subvolume(const char* path);

/* Create snapshot */
int btrfs_create_snapshot(const char* source, const char* dest);

/* List subvolumes */
btrfs_subvolume_t* btrfs_list_subvolumes(const char* path);

#endif /* BTRFS_H */
