#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "overflow.h"
#include "validate.h"

/* Heap block header */
typedef struct heap_block {
    size_t size;
    bool free;
    struct heap_block* next;
    struct heap_block* prev;  /* For better coalescing */
} heap_block_t;

#define HEAP_START 0x200000  /* 2MB */
#define HEAP_SIZE  (10 * 1024 * 1024)  /* 10MB */
#define HEAP_END   (HEAP_START + HEAP_SIZE)

static heap_block_t* heap_head = NULL;
static spinlock_t heap_lock = SPINLOCK_INIT;

void heap_init(void) {
    /* Allocate heap space */
    heap_head = (heap_block_t*)HEAP_START;
    heap_head->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_head->free = true;
    heap_head->next = NULL;
    heap_head->prev = NULL;
    
    printk("Heap: Initialized at 0x%x, size %u KB\n", HEAP_START, HEAP_SIZE / 1024);
}

static void* heap_alloc_block(size_t size) {
    heap_block_t* current = heap_head;
    
    /* Align size to 8 bytes */
    size = ALIGN_UP(size, 8);
    
    if (size == 0) {
        DEBUG_WARN("Attempted to allocate 0 bytes");
        return NULL;
    }
    
    while (current) {
        if (current->free && current->size >= size) {
            /* Split block if large enough */
            if (current->size >= size + sizeof(heap_block_t) + 16) {
                heap_block_t* new_block = (heap_block_t*)((u8*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->free = true;
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) {
                    current->next->prev = new_block;
                }
                
                current->size = size;
                current->next = new_block;
            }
            
            current->free = false;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    
    DEBUG_WARN("Out of heap memory (requested %u bytes)", (u32)size);
    return NULL; /* Out of memory */
}

static void heap_coalesce(heap_block_t* block) {
    /* Coalesce with next block if free */
    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    /* Coalesce with previous block if free */
    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

static void heap_free_block(void* ptr) {
    if (!ptr) {
        DEBUG_WARN("Attempted to free NULL pointer");
        return;
    }
    
    /* Validate pointer is within heap */
    if ((u64)ptr < HEAP_START || (u64)ptr >= HEAP_END) {
        DEBUG_ERROR("Invalid pointer to free: 0x%p", ptr);
        return;
    }
    
    heap_block_t* block = (heap_block_t*)ptr - 1;
    
    /* Validate block header */
    if (block->size == 0 || block->size > HEAP_SIZE) {
        DEBUG_ERROR("Corrupted heap block at 0x%p (size: %u)", ptr, (u32)block->size);
        return;
    }
    
    if (block->free) {
        DEBUG_WARN("Double free detected at 0x%p", ptr);
        return;
    }
    
    block->free = true;
    heap_coalesce(block);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    spinlock_lock(&heap_lock);
    void* ptr = heap_alloc_block(size);
    spinlock_unlock(&heap_lock);
    
    return ptr;
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    spinlock_lock(&heap_lock);
    heap_free_block(ptr);
    spinlock_unlock(&heap_lock);
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    spinlock_lock(&heap_lock);
    
    heap_block_t* block = (heap_block_t*)ptr - 1;
    size_t old_size = block->size;
    
    /* If current block is large enough, just return it */
    if (old_size >= size) {
        spinlock_unlock(&heap_lock);
        return ptr;
    }
    
    spinlock_unlock(&heap_lock);
    
    /* Allocate new block and copy */
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size < size ? old_size : size);
        kfree(ptr);
    }
    return new_ptr;
}
