#include "robust_futex.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"

#define MAX_ROBUST_LISTS 1024

typedef struct robust_list_entry {
    process_t* process;
    robust_list_head_t head;
    struct robust_list_entry* next;
} robust_list_entry_t;

static robust_list_entry_t* robust_lists = NULL;
static spinlock_t robust_lock = SPINLOCK_INIT;

int set_robust_list(const robust_list_head_t* head, size_t len) {
    VALIDATE_PTR_RET(head, -1);
    
    if (len != sizeof(robust_list_head_t)) {
        DEBUG_ERROR("Invalid robust list size: %u", (u32)len);
        return -1;
    }
    
    process_t* proc = process_current();
    if (!proc) {
        return -1;
    }
    
    spinlock_lock(&robust_lock);
    
    /* Find or create entry */
    robust_list_entry_t* entry = robust_lists;
    while (entry) {
        if (entry->process == proc) {
            entry->head = *head;
            spinlock_unlock(&robust_lock);
            DEBUG_INFO("Robust list set for process %u", proc->pid);
            return 0;
        }
        entry = entry->next;
    }
    
    /* Create new entry */
    entry = (robust_list_entry_t*)kmalloc(sizeof(robust_list_entry_t));
    if (!entry) {
        spinlock_unlock(&robust_lock);
        return -1;
    }
    
    entry->process = proc;
    entry->head = *head;
    entry->next = robust_lists;
    robust_lists = entry;
    
    spinlock_unlock(&robust_lock);
    DEBUG_INFO("Robust list created for process %u", proc->pid);
    return 0;
}

int get_robust_list(robust_list_head_t* head, size_t* len) {
    VALIDATE_PTR_RET(head, -1);
    VALIDATE_PTR_RET(len, -1);
    
    process_t* proc = process_current();
    if (!proc) {
        return -1;
    }
    
    spinlock_lock(&robust_lock);
    
    robust_list_entry_t* entry = robust_lists;
    while (entry) {
        if (entry->process == proc) {
            *head = entry->head;
            *len = sizeof(robust_list_head_t);
            spinlock_unlock(&robust_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&robust_lock);
    return -1;
}

void robust_futex_exit(void) {
    process_t* proc = process_current();
    if (!proc) {
        return;
    }
    
    spinlock_lock(&robust_lock);
    
    robust_list_entry_t** prev = &robust_lists;
    robust_list_entry_t* entry = robust_lists;
    
    while (entry) {
        if (entry->process == proc) {
            /* Clean up robust futexes */
            robust_list_t* node = entry->head.list;
            while (node) {
                robust_list_t* next = node->next;
                /* Would wake waiters and mark as dead */
                node = next;
            }
            
            *prev = entry->next;
            kfree(entry);
            break;
        }
        prev = &entry->next;
        entry = entry->next;
    }
    
    spinlock_unlock(&robust_lock);
}
