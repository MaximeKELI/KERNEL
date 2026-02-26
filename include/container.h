#ifndef CONTAINER_H
#define CONTAINER_H

#include "types.h"
#include "namespace.h"
#include "cgroup.h"

/* Container */
typedef struct container {
    char name[64];
    u64 container_id;
    namespace_t* namespaces;
    cgroup_t* cgroup;
    void* rootfs;
    bool running;
    struct container* next;
} container_t;

/* Initialize container runtime */
void container_init(void);

/* Create container */
container_t* container_create(const char* name, const char* rootfs);

/* Start container */
int container_start(container_t* container);

/* Stop container */
int container_stop(container_t* container);

/* Delete container */
int container_delete(container_t* container);

/* List containers */
int container_list(container_t** containers, u32* count);

#endif /* CONTAINER_H */
