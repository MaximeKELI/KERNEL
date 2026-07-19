#include "tss.h"
#include "gdt.h"
#include "process.h"
#include "string.h"
#include "stdio.h"

/*
 * Long-mode Task State Segment. Only the stack-pointer fields matter: rsp0 is
 * loaded by the CPU on every ring 3 -> ring 0 transition. Packed so the field
 * offsets match the architectural layout exactly (104 bytes total).
 */
typedef struct __attribute__((packed)) {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} tss_t;

static tss_t tss __attribute__((aligned(16)));

/*
 * Fallback ring-0 stack, used for tasks that have no dedicated kernel stack
 * (the boot task) should they ever take a ring3 trap.
 */
static u8 tss_default_stack[16 * 1024] __attribute__((aligned(16)));

/*
 * Kernel stack top that SYSCALL switches to (SYSCALL, unlike interrupts, does
 * not consult the TSS). Kept in lock-step with tss.rsp0. Read from syscall_asm.S.
 */
u64 syscall_kernel_stack = 0;

/* Scratch cell used by syscall_asm.S to stash the user rsp during the stack
 * switch (mono-CPU; SYSCALL runs with interrupts masked so it is not reentered). */
u64 saved_user_rsp_tmp = 0;

static inline u64 stack_top(void* base, u64 size) {
    return ((u64)base + size) & ~0xFULL;
}

void tss_init(void) {
    memset(&tss, 0, sizeof(tss));
    u64 top = stack_top(tss_default_stack, sizeof(tss_default_stack));
    tss.rsp0 = top;
    tss.iomap_base = sizeof(tss_t);   /* no I/O bitmap */
    syscall_kernel_stack = top;

    gdt_install_tss((u64)(uintptr_t)&tss, sizeof(tss_t) - 1);
    __asm__ volatile("ltr %%ax" : : "a"((u16)GDT_TSS_SEL) : "memory");

    printk("[TSS] loaded (sel=0x%x, rsp0=0x%x)\n",
           (unsigned)GDT_TSS_SEL, (unsigned)top);
}

void tss_set_rsp0(u64 rsp0) {
    tss.rsp0 = rsp0;
    syscall_kernel_stack = rsp0;
}

void arch_update_kernel_stack(struct process* next) {
    u64 top;
    if (next && next->stack_base) {
        top = stack_top(next->stack_base, next->stack_size);
    } else {
        top = stack_top(tss_default_stack, sizeof(tss_default_stack));
    }
    tss.rsp0 = top;
    syscall_kernel_stack = top;
}
