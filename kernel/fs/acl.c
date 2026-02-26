#include "acl.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "fs/inode.h"

#define MAX_ACL_ENTRIES 64

typedef struct acl_entry_storage {
    u64 inode;
    acl_t acl;
    struct acl_entry_storage* next;
} acl_entry_storage_t;

static acl_entry_storage_t* acl_table = NULL;
static spinlock_t acl_lock = SPINLOCK_INIT;

static acl_entry_storage_t* acl_find_storage(u64 inode) {
    acl_entry_storage_t* storage = acl_table;
    while (storage) {
        if (storage->inode == inode) {
            return storage;
        }
        storage = storage->next;
    }
    return NULL;
}

int acl_set(const char* path, acl_t* acl) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(acl, -1);
    
    if (acl->entry_count > MAX_ACL_ENTRIES) {
        DEBUG_ERROR("Too many ACL entries: %u", acl->entry_count);
        return -1;
    }
    
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&acl_lock);
    
    acl_entry_storage_t* storage = acl_find_storage(inode);
    
    if (storage) {
        /* Free old ACL */
        acl_entry_t* entry = storage->acl.entries;
        while (entry) {
            acl_entry_t* next = entry->next;
            kfree(entry);
            entry = next;
        }
    } else {
        storage = (acl_entry_storage_t*)kmalloc(sizeof(acl_entry_storage_t));
        if (!storage) {
            spinlock_unlock(&acl_lock);
            return -1;
        }
        storage->inode = inode;
        storage->next = acl_table;
        acl_table = storage;
    }
    
    /* Copy ACL */
    storage->acl.entry_count = acl->entry_count;
    storage->acl.entries = NULL;
    
    acl_entry_t* src = acl->entries;
    acl_entry_t** dst = &storage->acl.entries;
    
    while (src) {
        *dst = (acl_entry_t*)kmalloc(sizeof(acl_entry_t));
        if (!*dst) {
            spinlock_unlock(&acl_lock);
            return -1;
        }
        **dst = *src;
        (*dst)->next = NULL;
        dst = &(*dst)->next;
        src = src->next;
    }
    
    spinlock_unlock(&acl_lock);
    DEBUG_INFO("ACL set: path=%s, entries=%u", path, acl->entry_count);
    return 0;
}

int acl_get(const char* path, acl_t* acl) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(acl, -1);
    
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&acl_lock);
    
    acl_entry_storage_t* storage = acl_find_storage(inode);
    if (!storage) {
        spinlock_unlock(&acl_lock);
        return -1; /* Would set ENOATTR */
    }
    
    /* Copy ACL */
    acl->entry_count = storage->acl.entry_count;
    acl->entries = NULL;
    
    acl_entry_t* src = storage->acl.entries;
    acl_entry_t** dst = &acl->entries;
    
    while (src) {
        *dst = (acl_entry_t*)kmalloc(sizeof(acl_entry_t));
        if (!*dst) {
            acl_free(acl);
            spinlock_unlock(&acl_lock);
            return -1;
        }
        **dst = *src;
        (*dst)->next = NULL;
        dst = &(*dst)->next;
        src = src->next;
    }
    
    spinlock_unlock(&acl_lock);
    return 0;
}

bool acl_check(const char* path, u32 uid, u32 gid, u8 requested_perms) {
    VALIDATE_STRING(path, 256);
    
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&acl_lock);
    
    acl_entry_storage_t* storage = acl_find_storage(inode);
    if (!storage) {
        spinlock_unlock(&acl_lock);
        return true; /* No ACL, use default permissions */
    }
    
    acl_entry_t* entry = storage->acl.entries;
    u8 effective_perms = 0;
    
    while (entry) {
        if (entry->tag_type == ACL_USER_OBJ && entry->id == uid) {
            effective_perms = entry->perms;
            break;
        } else if (entry->tag_type == ACL_USER && entry->id == uid) {
            effective_perms = entry->perms;
            break;
        } else if (entry->tag_type == ACL_GROUP_OBJ && entry->id == gid) {
            effective_perms |= entry->perms;
        } else if (entry->tag_type == ACL_GROUP && entry->id == gid) {
            effective_perms |= entry->perms;
        } else if (entry->tag_type == ACL_OTHER) {
            effective_perms |= entry->perms;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&acl_lock);
    
    return (effective_perms & requested_perms) == requested_perms;
}

void acl_free(acl_t* acl) {
    if (!acl) return;
    
    acl_entry_t* entry = acl->entries;
    while (entry) {
        acl_entry_t* next = entry->next;
        kfree(entry);
        entry = next;
    }
    
    acl->entries = NULL;
    acl->entry_count = 0;
}
