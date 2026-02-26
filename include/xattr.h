#ifndef XATTR_H
#define XATTR_H

#include "types.h"

/* Extended attribute namespaces */
#define XATTR_USER_PREFIX     "user."
#define XATTR_SYSTEM_PREFIX   "system."
#define XATTR_SECURITY_PREFIX "security."
#define XATTR_TRUSTED_PREFIX  "trusted."

/* Extended attribute flags */
#define XATTR_CREATE  0x01
#define XATTR_REPLACE 0x02

/* Set extended attribute */
int xattr_set(const char* path, const char* name, const void* value, size_t size, u32 flags);

/* Get extended attribute */
ssize_t xattr_get(const char* path, const char* name, void* value, size_t size);

/* Remove extended attribute */
int xattr_remove(const char* path, const char* name);

/* List extended attributes */
ssize_t xattr_list(const char* path, char* list, size_t size);

#endif /* XATTR_H */
