#ifndef DENTRY_H
#define DENTRY_H

#include "types.h"
#include "inode.h"
#include "refcount.h"

/* Directory entry (dentry) */
typedef struct dentry {
    refcount_t refcount;  /* Reference counting */
    char* name;
    inode_t* inode;
    struct dentry* parent;
    struct dentry* child;
    struct dentry* sibling;
} dentry_t;

/* Initialize dentry system */
void dentry_init(void);

/* Allocate dentry */
dentry_t* dentry_alloc(const char* name, inode_t* inode);

/* Free dentry */
void dentry_free(dentry_t* dentry);

/* Lookup dentry */
dentry_t* dentry_lookup(dentry_t* parent, const char* name);

/* Add child dentry */
int dentry_add_child(dentry_t* parent, dentry_t* child);

/* Remove child dentry */
int dentry_remove_child(dentry_t* parent, const char* name);

/* Get dentry */
dentry_t* dentry_get(dentry_t* dentry);

/* Put dentry */
void dentry_put(dentry_t* dentry);

#endif /* DENTRY_H */
