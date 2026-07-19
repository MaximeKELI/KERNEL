#ifndef MM_H
#define MM_H

#include "types.h"

/*
 * Minimal Linux-style virtual-memory description: an mm_struct owns a sorted
 * list of VMAs (virtual memory areas). The page-fault handler consults it to
 * demand-page anonymous memory, grow the stack, and copy-on-write, instead of
 * eagerly mapping everything up front.
 */

/* VMA protection / behaviour flags (mirror Linux VM_*). */
#define VM_READ      0x00000001
#define VM_WRITE     0x00000002
#define VM_EXEC      0x00000004
#define VM_GROWSDOWN 0x00000100   /* stack: a fault just below start grows it */
#define VM_ANON      0x00000200   /* anonymous demand-zero backing */

/* User address-space layout for brk()/mmap() (all inside the user PDPT[1]). */
#define USER_BRK_BASE   0x41000000UL    /* 1 GiB + 16 MiB */
#define USER_BRK_MAX    0x48000000UL
#define USER_MMAP_LOW   0x50000000UL    /* 1.25 GiB, grows up */
#define USER_MMAP_HIGH  0x70000000UL

typedef struct vma {
    u64 start;        /* page-aligned, inclusive */
    u64 end;          /* page-aligned, exclusive */
    u64 vm_flags;     /* VM_* */
    struct vma* next; /* sorted ascending by start */
} vma_t;

typedef struct mm_struct {
    vma_t* vmas;
    u64 brk_start;
    u64 brk;
    u64 mmap_base;    /* next free mmap hint (grows up) */
} mm_struct_t;

mm_struct_t* mm_create(void);
void mm_destroy(mm_struct_t* mm);

vma_t* mm_find_vma(mm_struct_t* mm, u64 addr);
vma_t* mm_insert_vma(mm_struct_t* mm, u64 start, u64 end, u64 vm_flags);
int    mm_remove_range(mm_struct_t* mm, u64 start, u64 end);   /* munmap */
int    mm_protect_range(mm_struct_t* mm, u64 start, u64 end, u64 vm_flags);

/* Page-fault dispatcher. Returns 0 if the fault was serviced, -1 otherwise. */
int mm_fault(mm_struct_t* mm, u64 addr, u64 error_code);

/* brk()/mmap() backends. Return new break / mapping address, or (u64)-1. */
u64 mm_brk(mm_struct_t* mm, u64 new_brk);
u64 mm_mmap(mm_struct_t* mm, u64 addr, u64 length, u64 vm_flags);

#endif /* MM_H */
