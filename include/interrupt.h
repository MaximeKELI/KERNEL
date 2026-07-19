#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

/* Interrupt handler type */
typedef void (*irq_handler_t)(u32 irq, void* data);

/*
 * Saved CPU state at the point of an interrupt/exception.
 * Layout MUST match the push order in isr_common (kernel/interrupt/idt_asm.S):
 * r15..r8, rbp, rdi, rsi, rdx, rcx, rbx, rax pushed by the stub, then the
 * int number + error code pushed by the ISR entry, then the hardware frame
 * (rip, cs, rflags, rsp, ss) pushed by the CPU.
 */
typedef struct interrupt_frame {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 int_no;
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} interrupt_frame_t;

/* Initialize interrupt subsystem */
void interrupt_init(void);

/* Register IRQ handler */
void irq_register(u32 irq, irq_handler_t handler, void* data);

/* Unregister IRQ handler */
void irq_unregister(u32 irq);

/* Enable/disable interrupts */
void enable_interrupts(void);
void disable_interrupts(void);
bool interrupts_enabled(void);

/* Global interrupt counter for AI monitoring */
extern u64 global_interrupt_count;

/* Global I/O statistics for AI monitoring */
extern u64 global_io_read_bytes;
extern u64 global_io_write_bytes;
extern u64 global_io_read_ops;
extern u64 global_io_write_ops;

/* Global network statistics for AI monitoring */
extern u64 global_net_tx_bytes;
extern u64 global_net_rx_bytes;
extern u64 global_net_tx_packets;
extern u64 global_net_rx_packets;

/* PIC */
void pic_init(void);
void pic_enable_irq(u32 irq);
void pic_disable_irq(u32 irq);
void pic_send_eoi(u32 irq);

/* Exception handlers */
void exception_handler(u32 vector, u64 error_code, interrupt_frame_t* frame);

#endif /* INTERRUPT_H */
