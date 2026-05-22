#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "types.h"

/* Namespace types */
#define CLONE_NEWNS  0x00020000
#define CLONE_NEWPID 0x20000000
#define CLONE_NEWNET 0x40000000
#define CLONE_NEWIPC 0x08000000
#define CLONE_NEWUTS 0x04000000
#define CLONE_NEWUSER 0x10000000

/* Namespace structure */
typedef struct namespace {
    u64 type;
    u64 id;
    void* private_data;
    struct namespace* next;
} namespace_t;

/* Initialize namespaces */
void namespace_init(void);

/* Create namespace */
namespace_t* namespace_create(u64 type);

/* Destroy namespace */
void namespace_destroy(namespace_t* ns);

/* Get namespace */
namespace_t* namespace_get(u64 type);

/* Set namespace */
int namespace_set(u64 type, namespace_t* ns);

/* Unshare namespace */
int unshare(int flags);

/* Setns */
int setns(int fd, int nstype);

#endif /* NAMESPACE_H */
