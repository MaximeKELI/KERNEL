#include "interrupt.h"
#include "stdio.h"
#include "kernel.h"

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
    const char* name = "Unknown";
    if (vector < 21) {
        name = exception_names[vector];
    }
    
    printk("\n!!! EXCEPTION: %s (vector %u, error code 0x%x) !!!\n",
           name, vector, (u32)error_code);
    
    if (vector == 14) {
        u64 cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        extern int cow_handle_page_fault(u64 cr2, u64 error_code);
        if (cow_handle_page_fault(cr2, error_code) == 0) {
            return;
        }
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
