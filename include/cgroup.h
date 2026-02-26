#ifndef CGROUP_H
#define CGROUP_H

#include "types.h"

/* Cgroup subsystem types */
#define CGROUP_CPU    1
#define CGROUP_MEMORY 2
#define CGROUP_IO     4
#define CGROUP_PIDS   8

/* Cgroup structure */
typedef struct cgroup {
    char name[64];
    u32 id;
    u64 cpu_limit;
    u64 memory_limit;
    u64 io_limit;
    u32 pids_limit;
    u32 process_count;
    struct cgroup* parent;
    struct cgroup* children;
    struct cgroup* sibling;
} cgroup_t;

/* Initialize cgroups */
void cgroup_init(void);

/* Create cgroup */
cgroup_t* cgroup_create(const char* name, cgroup_t* parent);

/* Attach process to cgroup */
int cgroup_attach_process(cgroup_t* cg, u64 pid);

/* Set resource limits */
int cgroup_set_cpu_limit(cgroup_t* cg, u64 limit);
int cgroup_set_memory_limit(cgroup_t* cg, u64 limit);
int cgroup_set_pids_limit(cgroup_t* cg, u32 limit);

/* Get cgroup for process */
cgroup_t* cgroup_get_process(u64 pid);

#endif /* CGROUP_H */
