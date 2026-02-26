#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

/* Interrupt handler type */
typedef void (*irq_handler_t)(u32 irq, void* data);

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

/* PIC */
void pic_init(void);
void pic_enable_irq(u32 irq);
void pic_disable_irq(u32 irq);
void pic_send_eoi(u32 irq);

/* Exception handlers */
void exception_handler(u32 vector, u64 error_code);

#endif /* INTERRUPT_H */
