#include "interrupt.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

#define ICW1_ICW4    0x01
#define ICW1_SINGLE  0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL   0x08
#define ICW1_INIT    0x10

#define ICW4_8086    0x01
#define ICW4_AUTO    0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM    0x10

/* IRQ handlers */
irq_handler_t irq_handlers[16] = {0};
void* irq_data[16] = {0};

void pic_init(void) {
    /* Save masks */
    u8 a1 = inb(PIC1_DATA);
    u8 a2 = inb(PIC2_DATA);
    
    /* Initialize PIC1 */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, 0x20);  /* IRQ 0-7 mapped to 0x20-0x27 */
    outb(PIC1_DATA, 4);     /* PIC2 at IRQ2 */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC1_DATA, a1);
    
    /* Initialize PIC2 */
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_DATA, 0x28);  /* IRQ 8-15 mapped to 0x28-0x2F */
    outb(PIC2_DATA, 2);
    outb(PIC2_DATA, ICW4_8086);
    outb(PIC2_DATA, a2);
    
    /* Mask all interrupts */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_enable_irq(u32 irq) {
    u16 port;
    u8 value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_disable_irq(u32 irq) {
    u16 port;
    u8 value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_send_eoi(u32 irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void irq_register(u32 irq, irq_handler_t handler, void* data) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
        irq_data[irq] = data;
        pic_enable_irq(irq);
    }
}

void irq_unregister(u32 irq) {
    if (irq < 16) {
        irq_handlers[irq] = NULL;
        irq_data[irq] = NULL;
        pic_disable_irq(irq);
    }
}
