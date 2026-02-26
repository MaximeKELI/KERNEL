#include "inode.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "refcount.h"
#include "validate.h"

#define MAX_INODES 1024
#define INODE_HASH_SIZE 256

static inode_t inode_cache[MAX_INODES];
static inode_t* inode_hash[INODE_HASH_SIZE];
static spinlock_t inode_lock = SPINLOCK_INIT;
static u64 next_ino = 1;

static u32 inode_hash_func(u64 ino) {
    return (u32)(ino % INODE_HASH_SIZE);
}

void inode_init(void) {
    memset(inode_cache, 0, sizeof(inode_cache));
    memset(inode_hash, 0, sizeof(inode_hash));
    DEBUG_INFO("Inode system initialized");
}

inode_t* inode_alloc(void) {
    spinlock_lock(&inode_lock);
    
    for (u32 i = 0; i < MAX_INODES; i++) {
        if (inode_cache[i].ino == 0) {
            inode_t* inode = &inode_cache[i];
            memset(inode, 0, sizeof(inode_t));
            inode->refcount = REFCOUNT_INIT;
            refcount_get(&inode->refcount); /* Initial reference */
            inode->ino = next_ino++;
            inode->nlink = 1;
            
            /* Add to hash */
            u32 hash = inode_hash_func(inode->ino);
            inode->private_data = (void*)inode_hash[hash];
            inode_hash[hash] = inode;
            
            spinlock_unlock(&inode_lock);
            return inode;
        }
    }
    
    spinlock_unlock(&inode_lock);
    DEBUG_ERROR("Out of inodes");
    return NULL;
}

void inode_free(inode_t* inode) {
    VALIDATE_PTR_VOID(inode);
    
    spinlock_lock(&inode_lock);
    
    /* Remove from hash */
    u32 hash = inode_hash_func(inode->ino);
    inode_t** prev = &inode_hash[hash];
    while (*prev) {
        if (*prev == inode) {
            *prev = (inode_t*)(*prev)->private_data;
            break;
        }
        prev = (inode_t**)(*prev)->private_data;
    }
    
    memset(inode, 0, sizeof(inode_t));
    
    spinlock_unlock(&inode_lock);
}

inode_t* inode_get(u64 ino) {
    spinlock_lock(&inode_lock);
    
    u32 hash = inode_hash_func(ino);
    inode_t* inode = inode_hash[hash];
    
    while (inode) {
        if (inode->ino == ino) {
            inode->nlink++;
            refcount_get(&inode->refcount); /* Increment reference */
            spinlock_unlock(&inode_lock);
            return inode;
        }
        inode = (inode_t*)inode->private_data;
    }
    
    spinlock_unlock(&inode_lock);
    return NULL;
}

void inode_put(inode_t* inode) {
    VALIDATE_PTR_VOID(inode);
    
    spinlock_lock(&inode_lock);
    if (inode->nlink > 0) {
        inode->nlink--;
    }
    
    /* Decrement reference count */
    u32 refcount = refcount_put(&inode->refcount);
    
    /* If no more references, free the inode */
    if (refcount == 0 && inode->nlink == 0) {
        spinlock_unlock(&inode_lock);
        inode_free(inode);
        return;
    }
    
    spinlock_unlock(&inode_lock);
}
