#ifndef ACL_H
#define ACL_H

#include "types.h"

/* ACL entry types */
#define ACL_USER_OBJ     0x01
#define ACL_USER         0x02
#define ACL_GROUP_OBJ    0x04
#define ACL_GROUP        0x08
#define ACL_MASK         0x10
#define ACL_OTHER        0x20

/* ACL permissions */
#define ACL_READ         0x04
#define ACL_WRITE        0x02
#define ACL_EXECUTE      0x01

/* ACL entry */
typedef struct acl_entry {
    u8 tag_type;
    u32 id;  /* User or group ID */
    u8 perms;
    struct acl_entry* next;
} acl_entry_t;

/* ACL structure */
typedef struct acl {
    acl_entry_t* entries;
    u32 entry_count;
} acl_t;

/* Set ACL */
int acl_set(const char* path, acl_t* acl);

/* Get ACL */
int acl_get(const char* path, acl_t* acl);

/* Check ACL permission */
bool acl_check(const char* path, u32 uid, u32 gid, u8 requested_perms);

/* Free ACL */
void acl_free(acl_t* acl);

#endif /* ACL_H */
