#include "interrupt.h"
#include "stdio.h"
#include "kernel.h"
#include "signal.h"
#include "process.h"

static const char* exception_names[] = {
    "Divide by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception"
};

void exception_handler(u32 vector, u64 error_code, interrupt_frame_t* frame) {
    /*
     * Page faults are normal control flow (COW, demand paging, stack growth):
     * try to service them silently first. Only fall through to the crash path
     * for genuinely unrecoverable faults.
     */
    u64 cr2 = 0;
    if (vector == 14) {
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        extern int cow_handle_page_fault(u64 cr2, u64 error_code);
        if (cow_handle_page_fault(cr2, error_code) == 0) {
            return;
        }
    }

    /*
     * A fault taken in ring 3 that we could not service is the process's fault,
     * not the kernel's: deliver the appropriate signal (default action = kill)
     * instead of panicking. signal_check_on_irq_return rewrites `frame` to enter
     * the handler; with no handler, signal_dispatch performs the default
     * terminate and never returns.
     */
    if (frame && (frame->cs & 3) == 3) {
        process_t* proc = process_current();
        if (proc) {
            int sig = SIGSEGV;
            if (vector == 6) {
                sig = SIGILL;            /* invalid opcode */
            } else if (vector == 0) {
                sig = SIGFPE;            /* divide error */
            } else if (vector == 13) {
                sig = SIGSEGV;           /* GP fault */
            }
            signal_force(proc, sig);
            extern void signal_check_on_irq_return(interrupt_frame_t*);
            signal_check_on_irq_return(frame);
            return;   /* resume: either into the handler or (default) terminated */
        }
    }

    const char* name = "Unknown";
    if (vector < 21) {
        name = exception_names[vector];
    }

    printk("\n!!! EXCEPTION: %s (vector %u, error code 0x%x) !!!\n",
           name, vector, (u32)error_code);

    if (vector == 14) {
        printk("Page fault at address: 0x%p (err=0x%x)\n", (void*)cr2, (u32)error_code);
    }
    
    /*
     * Dump the CPU state captured at the fault site (from the interrupt frame),
     * not the live registers of this handler.
     */
    if (frame) {
        printk("Registers (at fault):\n");
        printk("  RIP: 0x%p  CS: 0x%p  RFLAGS: 0x%p\n",
               (void*)frame->rip, (void*)frame->cs, (void*)frame->rflags);
        printk("  RAX: 0x%p  RBX: 0x%p  RCX: 0x%p  RDX: 0x%p\n",
               (void*)frame->rax, (void*)frame->rbx, (void*)frame->rcx, (void*)frame->rdx);
        printk("  RSI: 0x%p  RDI: 0x%p  RBP: 0x%p  RSP: 0x%p\n",
               (void*)frame->rsi, (void*)frame->rdi, (void*)frame->rbp, (void*)frame->rsp);
        printk("  R8 : 0x%p  R9 : 0x%p  R10: 0x%p  R11: 0x%p\n",
               (void*)frame->r8, (void*)frame->r9, (void*)frame->r10, (void*)frame->r11);
        printk("  R12: 0x%p  R13: 0x%p  R14: 0x%p  R15: 0x%p\n",
               (void*)frame->r12, (void*)frame->r13, (void*)frame->r14, (void*)frame->r15);
    }
    
    panic("Kernel exception");
}
