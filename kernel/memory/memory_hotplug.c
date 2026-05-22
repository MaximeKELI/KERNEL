#include "memory_hotplug.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "pmm.h"
#include "vmm.h"

static memory_block_t* memory_blocks = NULL;
static spinlock_t hotplug_lock = SPINLOCK_INIT;

int memory_block_add(void* start, size_t size, u32 node_id) {
    VALIDATE_PTR_RET(start, -1);
    
    if (size == 0 || size % PAGE_SIZE != 0) {
        DEBUG_ERROR("Invalid memory block size: %u", (u32)size);
        return -1;
    }
    
    spinlock_lock(&hotplug_lock);
    
    /* Check for overlap */
    memory_block_t* block = memory_blocks;
    while (block) {
        if ((start >= block->start && start < (void*)((u64)block->start + block->size)) ||
            (block->start >= start && block->start < (void*)((u64)start + size))) {
            spinlock_unlock(&hotplug_lock);
            DEBUG_ERROR("Memory block overlap detected");
            return -1;
        }
        block = block->next;
    }
    
    /* Create new block */
    block = (memory_block_t*)kmalloc(sizeof(memory_block_t));
    if (!block) {
        spinlock_unlock(&hotplug_lock);
        return -1;
    }
    
    block->start = start;
    block->size = size;
    block->state = MEMORY_STATE_OFFLINE;
    block->node_id = node_id;
    block->next = memory_blocks;
    memory_blocks = block;
    
    spinlock_unlock(&hotplug_lock);
    
    DEBUG_INFO("Memory block added: start=0x%p, size=%u KB, node=%u", 
               start, (u32)(size / 1024), node_id);
    return 0;
}

int memory_block_remove(void* start) {
    VALIDATE_PTR_RET(start, -1);
    
    spinlock_lock(&hotplug_lock);
    
    memory_block_t** prev = &memory_blocks;
    memory_block_t* block = memory_blocks;
    
    while (block) {
        if (block->start == start) {
            if (block->state != MEMORY_STATE_OFFLINE) {
                spinlock_unlock(&hotplug_lock);
                DEBUG_ERROR("Cannot remove online memory block");
                return -1;
            }
            
            *prev = block->next;
            kfree(block);
            spinlock_unlock(&hotplug_lock);
            
            DEBUG_INFO("Memory block removed: start=0x%p", start);
            return 0;
        }
        prev = &block->next;
        block = block->next;
    }
    
    spinlock_unlock(&hotplug_lock);
    return -1;
}

int memory_block_online(void* start) {
    VALIDATE_PTR_RET(start, -1);
    
    spinlock_lock(&hotplug_lock);
    
    memory_block_t* block = memory_blocks;
    while (block) {
        if (block->start == start) {
            if (block->state == MEMORY_STATE_ONLINE) {
                spinlock_unlock(&hotplug_lock);
                return 0; /* Already online */
            }
            
            block->state = MEMORY_STATE_ONLINE;
            
            /* Add to PMM - would call pmm_add_range(block->start, block->size / PAGE_SIZE) */
            
            spinlock_unlock(&hotplug_lock);
            DEBUG_INFO("Memory block onlined: start=0x%p, size=%u KB", 
                       start, (u32)(block->size / 1024));
            return 0;
        }
        block = block->next;
    }
    
    spinlock_unlock(&hotplug_lock);
    return -1;
}

int memory_block_offline(void* start) {
    VALIDATE_PTR_RET(start, -1);
    
    spinlock_lock(&hotplug_lock);
    
    memory_block_t* block = memory_blocks;
    while (block) {
        if (block->start == start) {
            if (block->state == MEMORY_STATE_OFFLINE) {
                spinlock_unlock(&hotplug_lock);
                return 0; /* Already offline */
            }
            
            block->state = MEMORY_STATE_GOING_OFFLINE;
            
            /* Migrate pages away from this block */
            /* Would implement page migration here */
            
            block->state = MEMORY_STATE_OFFLINE;
            
            /* Remove from PMM */
            /* Would call pmm_remove_range(block->start, pages) */
            
            spinlock_unlock(&hotplug_lock);
            DEBUG_INFO("Memory block offlined: start=0x%p", start);
            return 0;
        }
        block = block->next;
    }
    
    spinlock_unlock(&hotplug_lock);
    return -1;
}

u32 memory_block_get_state(void* start) {
    VALIDATE_PTR_RET(start, MEMORY_STATE_OFFLINE);
    
    spinlock_lock(&hotplug_lock);
    
    memory_block_t* block = memory_blocks;
    while (block) {
        if (block->start == start) {
            u32 state = block->state;
            spinlock_unlock(&hotplug_lock);
            return state;
        }
        block = block->next;
    }
    
    spinlock_unlock(&hotplug_lock);
    return MEMORY_STATE_OFFLINE;
}

memory_block_t* memory_block_list(void) {
    return memory_blocks;
}
