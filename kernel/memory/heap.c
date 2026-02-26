#include "memory.h"
#include "string.h"
#include "stdio.h"

/* Heap block header */
typedef struct heap_block {
    size_t size;
    bool free;
    struct heap_block* next;
} heap_block_t;

#define HEAP_START 0x200000  /* 2MB */
#define HEAP_SIZE  (10 * 1024 * 1024)  /* 10MB */
#define HEAP_END   (HEAP_START + HEAP_SIZE)

static heap_block_t* heap_head = NULL;

void heap_init(void) {
    /* Allocate heap space */
    heap_head = (heap_block_t*)HEAP_START;
    heap_head->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_head->free = true;
    heap_head->next = NULL;
    
    printk("Heap: Initialized at 0x%x, size %u KB\n", HEAP_START, HEAP_SIZE / 1024);
}

static void* heap_alloc_block(size_t size) {
    heap_block_t* current = heap_head;
    
    /* Align size to 8 bytes */
    size = ALIGN_UP(size, 8);
    
    while (current) {
        if (current->free && current->size >= size) {
            /* Split block if large enough */
            if (current->size >= size + sizeof(heap_block_t) + 8) {
                heap_block_t* new_block = (heap_block_t*)((u8*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->free = true;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->free = false;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    
    return NULL; /* Out of memory */
}

static void heap_free_block(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)ptr - 1;
    block->free = true;
    
    /* Merge with next block if free */
    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    return heap_alloc_block(size);
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void* ptr) {
    heap_free_block(ptr);
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    heap_block_t* block = (heap_block_t*)ptr - 1;
    if (block->size >= size) {
        return ptr;
    }
    
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size < size ? block->size : size);
        kfree(ptr);
    }
    return new_ptr;
}
