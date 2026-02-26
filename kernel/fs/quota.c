#include "quota.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

#define MAX_QUOTA_ENTRIES 1024

typedef struct quota_entry {
    u32 id;
    u32 type;
    quota_limits_t limits;
    bool active;
    struct quota_entry* next;
} quota_entry_t;

static quota_entry_t* quota_table = NULL;
static spinlock_t quota_lock = SPINLOCK_INIT;

int quota_init(void) {
    DEBUG_INFO("%s", "Quota system initialized");
    return 0;
}

int quota_set(u32 id, u32 type, const quota_limits_t* limits) {
    VALIDATE_PTR_RET(limits, -1);
    
    if (type > QUOTA_PROJECT) {
        DEBUG_ERROR("Invalid quota type: %u", type);
        return -1;
    }
    
    spinlock_lock(&quota_lock);
    
    /* Find or create entry */
    quota_entry_t* entry = quota_table;
    while (entry) {
        if (entry->id == id && entry->type == type) {
            entry->limits = *limits;
            spinlock_unlock(&quota_lock);
            DEBUG_INFO("Quota limits set: id=%u, type=%u", id, type);
            return 0;
        }
        entry = entry->next;
    }
    
    /* Create new entry */
    entry = (quota_entry_t*)kmalloc(sizeof(quota_entry_t));
    if (!entry) {
        spinlock_unlock(&quota_lock);
        return -1;
    }
    
    entry->id = id;
    entry->type = type;
    entry->limits = *limits;
    entry->active = true;
    entry->next = quota_table;
    quota_table = entry;
    
    spinlock_unlock(&quota_lock);
    DEBUG_INFO("Quota entry created: id=%u, type=%u", id, type);
    return 0;
}

int quota_get(u32 id, u32 type, quota_limits_t* limits) {
    VALIDATE_PTR_RET(limits, -1);
    
    spinlock_lock(&quota_lock);
    
    quota_entry_t* entry = quota_table;
    while (entry) {
        if (entry->id == id && entry->type == type) {
            *limits = entry->limits;
            spinlock_unlock(&quota_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&quota_lock);
    return -1;
}

int quota_check_block(u32 id, u32 type, u64 blocks) {
    quota_limits_t limits;
    if (quota_get(id, type, &limits) != 0) {
        return 0; /* No quota set */
    }
    
    if (limits.block_hard_limit > 0 && 
        (limits.block_used + blocks) > limits.block_hard_limit) {
        DEBUG_WARN("Quota block hard limit exceeded: id=%u, used=%llu, limit=%llu",
                  id, (unsigned long long)limits.block_used, 
                  (unsigned long long)limits.block_hard_limit);
        return -1;
    }
    
    return 0;
}

int quota_check_inode(u32 id, u32 type) {
    quota_limits_t limits;
    if (quota_get(id, type, &limits) != 0) {
        return 0; /* No quota set */
    }
    
    if (limits.inode_hard_limit > 0 && 
        limits.inode_used >= limits.inode_hard_limit) {
        DEBUG_WARN("Quota inode hard limit exceeded: id=%u", id);
        return -1;
    }
    
    return 0;
}

int quota_update_block(u32 id, u32 type, i64 delta) {
    spinlock_lock(&quota_lock);
    
    quota_entry_t* entry = quota_table;
    while (entry) {
        if (entry->id == id && entry->type == type) {
            if (delta > 0) {
                entry->limits.block_used += (u64)delta;
            } else {
                u64 abs_delta = (u64)(-delta);
                if (entry->limits.block_used > abs_delta) {
                    entry->limits.block_used -= abs_delta;
                } else {
                    entry->limits.block_used = 0;
                }
            }
            spinlock_unlock(&quota_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&quota_lock);
    return 0; /* No quota set */
}

int quota_update_inode(u32 id, u32 type, i64 delta) {
    spinlock_lock(&quota_lock);
    
    quota_entry_t* entry = quota_table;
    while (entry) {
        if (entry->id == id && entry->type == type) {
            if (delta > 0) {
                entry->limits.inode_used += (u64)delta;
            } else {
                u64 abs_delta = (u64)(-delta);
                if (entry->limits.inode_used > abs_delta) {
                    entry->limits.inode_used -= abs_delta;
                } else {
                    entry->limits.inode_used = 0;
                }
            }
            spinlock_unlock(&quota_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&quota_lock);
    return 0; /* No quota set */
}

int quota_get_usage(u32 id, u32 type, quota_limits_t* usage) {
    return quota_get(id, type, usage);
}
