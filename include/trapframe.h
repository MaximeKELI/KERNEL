#ifndef TRAPFRAME_H
#define TRAPFRAME_H

#include "types.h"

/*
 * Snapshot of the user register state saved by syscall_entry (syscall_asm.S) on
 * the kernel stack. The field order MUST match the push sequence there: it is
 * the block `rsp` points at right before `call syscall_handler`, and a pointer
 * to it is passed to syscall_handler as the 7th argument so fork() can clone the
 * caller's exact user context.
 */
typedef struct syscall_frame {
    u64 r9, r8, r10, rdx, rsi, rdi;   /* args / caller-saved (push order) */
    u64 r15, r14, r13, r12, rbp, rbx; /* callee-saved */
    u64 user_rip;                     /* rcx at SYSCALL entry */
    u64 user_rflags;                  /* r11 at SYSCALL entry */
    u64 user_rsp;                     /* caller's user stack pointer */
} syscall_frame_t;

/*
 * Register image consumed by fork_child_trampoline (context_switch.S): the
 * trampoline sets rsp to a fork_frame, pops every GP register in this order and
 * then `iretq`s the trailing 5-word inter-privilege frame back to ring 3.
 */
typedef struct fork_frame {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp;
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
    u64 rip, cs, rflags, rsp, ss;   /* iretq frame */
} fork_frame_t;

#endif /* TRAPFRAME_H */
