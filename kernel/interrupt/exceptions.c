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

void exception_handler(u32 vector, u64 error_code) {
    const char* name = "Unknown";
    if (vector < 21) {
        name = exception_names[vector];
    }
    
    printk("\n!!! EXCEPTION: %s (vector %u, error code 0x%x) !!!\n",
           name, vector, (u32)error_code);
    
    if (vector == 14) {
        /* Page fault */
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        printk("Page fault at address: 0x%p\n", (void*)cr2);
        printk("Error code: 0x%x\n", (u32)error_code);
    }
    
    /* Dump registers */
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, rip, rflags;
    asm volatile(
        "mov %%rax, %0\n\t"
        "mov %%rbx, %1\n\t"
        "mov %%rcx, %2\n\t"
        "mov %%rdx, %3\n\t"
        "mov %%rsi, %4\n\t"
        "mov %%rdi, %5\n\t"
        "mov %%rbp, %6\n\t"
        "mov %%rsp, %7"
        : "=r"(rax), "=r"(rbx), "=r"(rcx), "=r"(rdx),
          "=r"(rsi), "=r"(rdi), "=r"(rbp), "=r"(rsp)
    );
    asm volatile("lea (%%rip), %0" : "=r"(rip));
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    
    printk("Registers:\n");
    printk("  RAX: 0x%p  RBX: 0x%p  RCX: 0x%p  RDX: 0x%p\n",
           (void*)rax, (void*)rbx, (void*)rcx, (void*)rdx);
    printk("  RSI: 0x%p  RDI: 0x%p  RBP: 0x%p  RSP: 0x%p\n",
           (void*)rsi, (void*)rdi, (void*)rbp, (void*)rsp);
    printk("  RIP: 0x%p  RFLAGS: 0x%p\n", (void*)rip, (void*)rflags);
    
    panic("Kernel exception");
}
