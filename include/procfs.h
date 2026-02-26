#ifndef PROCFS_H
#define PROCFS_H

#include "types.h"
#include "fs/vfs.h"

/* Proc filesystem */
#define PROC_ROOT "/proc"

/* Initialize procfs */
int procfs_init(void);

/* Mount procfs */
int procfs_mount(const char* mountpoint);

/* Create proc entry */
typedef struct proc_entry {
    char name[64];
    vfs_file_t* file;
    struct proc_entry* parent;
    struct proc_entry* children;
    struct proc_entry* sibling;
} proc_entry_t;

/* Create proc file */
int proc_create(const char* path, vfs_file_t* file);

/* Remove proc file */
int proc_remove(const char* path);

/* Read proc file */
ssize_t proc_read(const char* path, void* buf, size_t count);

#endif /* PROCFS_H */
