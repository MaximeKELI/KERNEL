#ifndef TSS_H
#define TSS_H

#include "types.h"

struct process;

/*
 * Task State Segment (x86-64).
 *
 * In long mode the TSS no longer holds a hardware task context; the CPU only
 * consults it for the privilege-level stack pointers (rsp0..rsp2) and the
 * interrupt-stack table (ist1..ist7). rsp0 is THE critical field: on any
 * ring 3 -> ring 0 transition (interrupt, exception, fault) the CPU loads
 * rsp from tss.rsp0. Without a valid TSS + rsp0, a userspace interrupt has
 * no kernel stack and the CPU triple-faults.
 */

/* Install the TSS descriptor in the GDT and load it with ltr. */
void tss_init(void);

/*
 * Point the ring-0 trap stack (tss.rsp0) at `rsp0`. Also mirrored into the
 * SYSCALL kernel stack, since SYSCALL does not switch stacks on its own.
 */
void tss_set_rsp0(u64 rsp0);

/*
 * Make `next` the task whose kernel stack receives ring3 traps and syscalls.
 * Called from the scheduler on every context switch. Tasks with no dedicated
 * kernel stack (the boot task) fall back to the TSS's private stack.
 */
void arch_update_kernel_stack(struct process* next);

#endif /* TSS_H */
