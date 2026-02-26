#ifndef MEMORY_CONSTANTS_H
#define MEMORY_CONSTANTS_H

/* Memory layout constants */
#define KERNEL_BASE       0x100000  /* 1MB - Kernel start */
#define KERNEL_STACK_BASE 0x100000  /* Kernel stack (in boot.asm) */
#define HEAP_START        0x200000  /* 2MB - Heap start */
#define HEAP_SIZE         (10 * 1024 * 1024)  /* 10MB heap */
#define VGA_MEMORY        0xB8000   /* VGA text mode */
#define PAGE_TABLE_BASE   0x1000    /* Page tables */

/* Memory protection */
#define KERNEL_CODE_START 0x100000
#define KERNEL_CODE_END   0x400000
#define USER_SPACE_START  0x40000000
#define USER_SPACE_END    0x7FFFFFFFFFFF

/* Alignment */
#define CACHE_LINE_SIZE   64
#define PAGE_ALIGN(addr)  ALIGN_UP(addr, PAGE_SIZE)

#endif /* MEMORY_CONSTANTS_H */
