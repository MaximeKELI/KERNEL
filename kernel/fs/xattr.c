#include "xattr.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "fs/inode.h"

#define MAX_XATTR_ENTRIES 1024
#define MAX_XATTR_NAME_LEN 256
#define MAX_XATTR_VALUE_LEN 4096

typedef struct xattr_entry {
    u64 inode;
    char name[MAX_XATTR_NAME_LEN];
    void* value;
    size_t value_size;
    struct xattr_entry* next;
} xattr_entry_t;

static xattr_entry_t* xattr_table = NULL;
static spinlock_t xattr_lock = SPINLOCK_INIT;

static xattr_entry_t* xattr_find(u64 inode, const char* name) {
    xattr_entry_t* entry = xattr_table;
    while (entry) {
        if (entry->inode == inode && strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

int xattr_set(const char* path, const char* name, const void* value, size_t size, u32 flags) {
    VALIDATE_STRING(path, 256);
    VALIDATE_STRING(name, MAX_XATTR_NAME_LEN);
    VALIDATE_PTR_RET(value, -1);
    
    if (size > MAX_XATTR_VALUE_LEN) {
        DEBUG_ERROR("Xattr value too large: %u", (u32)size);
        return -1;
    }
    
    /* Get inode for path */
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&xattr_lock);
    
    xattr_entry_t* entry = xattr_find(inode, name);
    
    if (entry) {
        if (flags & XATTR_CREATE) {
            spinlock_unlock(&xattr_lock);
            return -1; /* Would set EEXIST */
        }
        
        /* Replace existing */
        if (entry->value_size != size) {
            kfree(entry->value);
            entry->value = kmalloc(size);
            if (!entry->value) {
                spinlock_unlock(&xattr_lock);
                return -1;
            }
        }
        memcpy(entry->value, value, size);
        entry->value_size = size;
    } else {
        if (flags & XATTR_REPLACE) {
            spinlock_unlock(&xattr_lock);
            return -1; /* Would set ENOATTR */
        }
        
        /* Create new */
        entry = (xattr_entry_t*)kmalloc(sizeof(xattr_entry_t));
        if (!entry) {
            spinlock_unlock(&xattr_lock);
            return -1;
        }
        
        entry->inode = inode;
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->value = kmalloc(size);
        if (!entry->value) {
            kfree(entry);
            spinlock_unlock(&xattr_lock);
            return -1;
        }
        memcpy(entry->value, value, size);
        entry->value_size = size;
        entry->next = xattr_table;
        xattr_table = entry;
    }
    
    spinlock_unlock(&xattr_lock);
    DEBUG_INFO("Xattr set: path=%s, name=%s, size=%u", path, name, (u32)size);
    return 0;
}

ssize_t xattr_get(const char* path, const char* name, void* value, size_t size) {
    VALIDATE_STRING(path, 256);
    VALIDATE_STRING(name, MAX_XATTR_NAME_LEN);
    VALIDATE_PTR_RET(value, -1);
    
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&xattr_lock);
    
    xattr_entry_t* entry = xattr_find(inode, name);
    if (!entry) {
        spinlock_unlock(&xattr_lock);
        return -1; /* Would set ENOATTR */
    }
    
    if (size < entry->value_size) {
        spinlock_unlock(&xattr_lock);
        return -1; /* Would set ERANGE */
    }
    
    memcpy(value, entry->value, entry->value_size);
    ssize_t ret = (ssize_t)entry->value_size;
    
    spinlock_unlock(&xattr_lock);
    return ret;
}

int xattr_remove(const char* path, const char* name) {
    VALIDATE_STRING(path, 256);
    VALIDATE_STRING(name, MAX_XATTR_NAME_LEN);
    
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&xattr_lock);
    
    xattr_entry_t** prev = &xattr_table;
    xattr_entry_t* entry = xattr_table;
    
    while (entry) {
        if (entry->inode == inode && strcmp(entry->name, name) == 0) {
            *prev = entry->next;
            kfree(entry->value);
            kfree(entry);
            spinlock_unlock(&xattr_lock);
            DEBUG_INFO("Xattr removed: path=%s, name=%s", path, name);
            return 0;
        }
        prev = &entry->next;
        entry = entry->next;
    }
    
    spinlock_unlock(&xattr_lock);
    return -1; /* Would set ENOATTR */
}

ssize_t xattr_list(const char* path, char* list, size_t size) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(list, -1);
    
    u64 inode = 0; /* Would get from path */
    
    spinlock_lock(&xattr_lock);
    
    size_t total_len = 0;
    xattr_entry_t* entry = xattr_table;
    
    while (entry) {
        if (entry->inode == inode) {
            size_t name_len = strlen(entry->name) + 1;
            if (total_len + name_len > size) {
                break;
            }
            memcpy(list + total_len, entry->name, name_len);
            total_len += name_len;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&xattr_lock);
    return (ssize_t)total_len;
}
