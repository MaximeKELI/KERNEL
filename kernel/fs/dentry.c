#include "dentry.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "refcount.h"
#include "validate.h"

#define MAX_DENTRIES 2048

static dentry_t dentry_cache[MAX_DENTRIES];
static spinlock_t dentry_lock = SPINLOCK_INIT;
static u32 next_dentry = 0;

void dentry_init(void) {
    memset(dentry_cache, 0, sizeof(dentry_cache));
    DEBUG_INFO("Dentry system initialized");
}

dentry_t* dentry_alloc(const char* name, inode_t* inode) {
    VALIDATE_STRING(name, 256);
    VALIDATE_PTR_RET(inode, NULL);
    
    if (next_dentry >= MAX_DENTRIES) {
        DEBUG_ERROR("Out of dentries");
        return NULL;
    }
    
    dentry_t* dentry = &dentry_cache[next_dentry++];
    size_t name_len = strlen(name) + 1;
    dentry->name = (char*)kmalloc(name_len);
    if (!dentry->name) {
        DEBUG_ERROR("Failed to allocate dentry name");
        return NULL;
    }
    
    strncpy(dentry->name, name, name_len - 1);
    dentry->name[name_len - 1] = '\0';
    dentry->inode = inode;
    dentry->refcount.count = 1;
    spinlock_init(&dentry->refcount.lock);
    dentry->parent = NULL;
    dentry->child = NULL;
    dentry->sibling = NULL;
    
    /* Increment inode reference */
    if (inode) {
        refcount_get(&inode->refcount);
    }
    
    return dentry;
}

void dentry_free(dentry_t* dentry) {
    VALIDATE_PTR_VOID(dentry);
    
    spinlock_lock(&dentry_lock);
    
    if (dentry->name) {
        kfree(dentry->name);
    }
    
    /* Decrement inode reference */
    if (dentry->inode) {
        inode_put(dentry->inode);
    }
    
    /* Remove from parent's children */
    if (dentry->parent) {
        if (dentry->parent->child == dentry) {
            dentry->parent->child = dentry->sibling;
        } else {
            dentry_t* sibling = dentry->parent->child;
            while (sibling && sibling->sibling != dentry) {
                sibling = sibling->sibling;
            }
            if (sibling) {
                sibling->sibling = dentry->sibling;
            }
        }
    }
    
    memset(dentry, 0, sizeof(dentry_t));
    
    spinlock_unlock(&dentry_lock);
}

dentry_t* dentry_lookup(dentry_t* parent, const char* name) {
    VALIDATE_PTR_RET(parent, NULL);
    VALIDATE_STRING(name, 256);
    
    spinlock_lock(&dentry_lock);
    
    dentry_t* child = parent->child;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            refcount_get(&child->refcount); /* Increment reference */
            spinlock_unlock(&dentry_lock);
            return child;
        }
        child = child->sibling;
    }
    
    spinlock_unlock(&dentry_lock);
    return NULL;
}

int dentry_add_child(dentry_t* parent, dentry_t* child) {
    VALIDATE_PTR(parent);
    VALIDATE_PTR(child);
    
    spinlock_lock(&dentry_lock);
    
    child->parent = parent;
    child->sibling = parent->child;
    parent->child = child;
    
    spinlock_unlock(&dentry_lock);
    return 0;
}

int dentry_remove_child(dentry_t* parent, const char* name) {
    VALIDATE_PTR(parent);
    VALIDATE_STRING(name, 256);
    
    spinlock_lock(&dentry_lock);
    
    if (parent->child && strcmp(parent->child->name, name) == 0) {
        dentry_t* to_remove = parent->child;
        parent->child = to_remove->sibling;
        dentry_free(to_remove);
        spinlock_unlock(&dentry_lock);
        return 0;
    }
    
    dentry_t* sibling = parent->child;
    while (sibling && sibling->sibling) {
        if (strcmp(sibling->sibling->name, name) == 0) {
            dentry_t* to_remove = sibling->sibling;
            sibling->sibling = to_remove->sibling;
            dentry_free(to_remove);
            spinlock_unlock(&dentry_lock);
            return 0;
        }
        sibling = sibling->sibling;
    }
    
    spinlock_unlock(&dentry_lock);
    return -1;
}

dentry_t* dentry_get(dentry_t* dentry) {
    VALIDATE_PTR_RET(dentry, NULL);
    
    spinlock_lock(&dentry_lock);
    refcount_get(&dentry->refcount); /* Increment reference */
    spinlock_unlock(&dentry_lock);
    
    return dentry;
}

void dentry_put(dentry_t* dentry) {
    VALIDATE_PTR_VOID(dentry);
    
    spinlock_lock(&dentry_lock);
    u32 refcount = refcount_put(&dentry->refcount);
    
    /* If no more references, free the dentry */
    if (refcount == 0) {
        spinlock_unlock(&dentry_lock);
        dentry_free(dentry);
        return;
    }
    
    spinlock_unlock(&dentry_lock);
}
