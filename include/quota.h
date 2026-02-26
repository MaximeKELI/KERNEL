#ifndef QUOTA_H
#define QUOTA_H

#include "types.h"

/* Quota types */
#define QUOTA_USER  0
#define QUOTA_GROUP 1
#define QUOTA_PROJECT 2

/* Quota limits */
typedef struct quota_limits {
    u64 block_hard_limit;
    u64 block_soft_limit;
    u64 inode_hard_limit;
    u64 inode_soft_limit;
    u64 block_used;
    u64 inode_used;
} quota_limits_t;

/* Initialize quota system */
int quota_init(void);

/* Set quota limits */
int quota_set(u32 id, u32 type, const quota_limits_t* limits);

/* Get quota limits */
int quota_get(u32 id, u32 type, quota_limits_t* limits);

/* Check quota before allocation */
int quota_check_block(u32 id, u32 type, u64 blocks);
int quota_check_inode(u32 id, u32 type);

/* Update quota usage */
int quota_update_block(u32 id, u32 type, i64 delta);
int quota_update_inode(u32 id, u32 type, i64 delta);

/* Get quota usage */
int quota_get_usage(u32 id, u32 type, quota_limits_t* usage);

#endif /* QUOTA_H */
