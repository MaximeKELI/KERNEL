#include "dentry.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_DENTRIES 2048

static dentry_t dentry_cache[MAX_DENTRIES];
static spinlock_t dentry_lock = SPINLOCK_INIT;
static u32 next_dentry = 0;

void dentry_init(void) {
    memset(dentry_cache, 0, sizeof(dentry_cache));
    DEBUG_INFO("Dentry system initialized");
}

dentry_t* dentry_alloc(const char* name, inode_t* inode) {
    if (next_dentry >= MAX_DENTRIES) {
        DEBUG_ERROR("Out of dentries");
        return NULL;
    }
    
    dentry_t* dentry = &dentry_cache[next_dentry++];
    dentry->name = (char*)kmalloc(strlen(name) + 1);
    if (!dentry->name) {
        DEBUG_ERROR("Failed to allocate dentry name");
        return NULL;
    }
    
    strncpy(dentry->name, name, sizeof(dentry->name) - 1);
    dentry->name[sizeof(dentry->name) - 1] = '\0';
    dentry->inode = inode;
    dentry->refcount = 1;
    dentry->parent = NULL;
    dentry->child = NULL;
    dentry->sibling = NULL;
    
    return dentry;
}

void dentry_free(dentry_t* dentry) {
    if (!dentry) return;
    
    spinlock_lock(&dentry_lock);
    
    if (dentry->name) {
        kfree(dentry->name);
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
    if (!parent || !name) return NULL;
    
    spinlock_lock(&dentry_lock);
    
    dentry_t* child = parent->child;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            child->refcount++;
            spinlock_unlock(&dentry_lock);
            return child;
        }
        child = child->sibling;
    }
    
    spinlock_unlock(&dentry_lock);
    return NULL;
}

int dentry_add_child(dentry_t* parent, dentry_t* child) {
    if (!parent || !child) return -1;
    
    spinlock_lock(&dentry_lock);
    
    child->parent = parent;
    child->sibling = parent->child;
    parent->child = child;
    
    spinlock_unlock(&dentry_lock);
    return 0;
}

int dentry_remove_child(dentry_t* parent, const char* name) {
    if (!parent || !name) return -1;
    
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
    if (!dentry) return NULL;
    
    spinlock_lock(&dentry_lock);
    dentry->refcount++;
    spinlock_unlock(&dentry_lock);
    
    return dentry;
}

void dentry_put(dentry_t* dentry) {
    if (!dentry) return;
    
    spinlock_lock(&dentry_lock);
    if (dentry->refcount > 0) {
        dentry->refcount--;
        if (dentry->refcount == 0) {
            dentry_free(dentry);
        }
    }
    spinlock_unlock(&dentry_lock);
}
