#include "interrupt.h"
#include "stdio.h"
#include "io.h"
#include "debug.h"

/* IDT entry */
typedef struct __packed {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 flags;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} idt_entry_t;

/* IDT pointer */
typedef struct __packed {
    u16 limit;
    u64 base;
} idt_ptr_t;

#define IDT_SIZE 256
static idt_entry_t idt[IDT_SIZE];
static idt_ptr_t idt_ptr;

/* Interrupt handler stubs (defined in idt_asm.S) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);
extern void syscall_handler_asm(void);

/* Common interrupt handler */
void interrupt_handler(u64 vector, u64 error_code) {
    if (vector < 32) {
        /* Exception */
        exception_handler(vector, error_code);
    } else if (vector >= 32 && vector < 48) {
        /* IRQ */
        u32 irq = vector - 32;
        pic_send_eoi(irq);
        
        /* Call registered handler */
        extern irq_handler_t irq_handlers[16];
        extern void* irq_data[16];
        if (irq_handlers[irq]) {
            irq_handlers[irq](irq, irq_data[irq]);
        }
    } else if (vector == 0x80) {
        /* System call - handled directly by syscall_entry in syscall_asm.S */
        /* This should not be reached if syscall_entry is properly configured */
        DEBUG_ERROR("System call reached interrupt handler (should use syscall_entry)");
    }
}

/* Set IDT entry */
static void idt_set_entry(u8 num, void* handler, u8 flags) {
    u64 addr = (u64)handler;
    idt[num].offset_low = (u16)(addr & 0xFFFF);
    idt[num].offset_mid = (u16)((addr >> 16) & 0xFFFF);
    idt[num].offset_high = (u32)((addr >> 32) & 0xFFFFFFFF);
    idt[num].selector = 0x08;  /* Kernel code segment */
    idt[num].ist = 0;
    idt[num].flags = flags;
    idt[num].zero = 0;
}

void interrupt_init(void) {
    /* Initialize IDT */
    memset(idt, 0, sizeof(idt));
    
    /* Set exception handlers */
    idt_set_entry(0, isr0, 0x8E);  /* Divide by zero */
    idt_set_entry(1, isr1, 0x8E);  /* Debug */
    idt_set_entry(2, isr2, 0x8E);  /* NMI */
    idt_set_entry(3, isr3, 0x8E);  /* Breakpoint */
    idt_set_entry(4, isr4, 0x8E);  /* Overflow */
    idt_set_entry(5, isr5, 0x8E);  /* Bound range */
    idt_set_entry(6, isr6, 0x8E);  /* Invalid opcode */
    idt_set_entry(7, isr7, 0x8E);  /* Device not available */
    idt_set_entry(8, isr8, 0x8E);  /* Double fault */
    idt_set_entry(10, isr10, 0x8E); /* Invalid TSS */
    idt_set_entry(11, isr11, 0x8E); /* Segment not present */
    idt_set_entry(12, isr12, 0x8E); /* Stack fault */
    idt_set_entry(13, isr13, 0x8E); /* General protection */
    idt_set_entry(14, isr14, 0x8E); /* Page fault */
    idt_set_entry(16, isr16, 0x8E); /* x87 FPU error */
    idt_set_entry(17, isr17, 0x8E); /* Alignment check */
    idt_set_entry(18, isr18, 0x8E); /* Machine check */
    idt_set_entry(19, isr19, 0x8E); /* SIMD FPU error */
    idt_set_entry(20, isr20, 0x8E); /* Virtualization */
    
    /* Set IRQ handlers */
    idt_set_entry(32, irq0, 0x8E);
    idt_set_entry(33, irq1, 0x8E);
    idt_set_entry(34, irq2, 0x8E);
    idt_set_entry(35, irq3, 0x8E);
    idt_set_entry(36, irq4, 0x8E);
    idt_set_entry(37, irq5, 0x8E);
    idt_set_entry(38, irq6, 0x8E);
    idt_set_entry(39, irq7, 0x8E);
    idt_set_entry(40, irq8, 0x8E);
    idt_set_entry(41, irq9, 0x8E);
    idt_set_entry(42, irq10, 0x8E);
    idt_set_entry(43, irq11, 0x8E);
    idt_set_entry(44, irq12, 0x8E);
    idt_set_entry(45, irq13, 0x8E);
    idt_set_entry(46, irq14, 0x8E);
    idt_set_entry(47, irq15, 0x8E);
    
    /* System call handler */
    idt_set_entry(0x80, syscall_handler_asm, 0xEE);  /* User mode accessible */
    
    /* Load IDT */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (u64)idt;
    asm volatile("lidt %0" : : "m"(idt_ptr));
    
    /* Initialize PIC */
    pic_init();
    
    printk("IDT: Initialized with %d entries\n", IDT_SIZE);
}

void enable_interrupts(void) {
    asm volatile("sti");
}

void disable_interrupts(void) {
    asm volatile("cli");
}

bool interrupts_enabled(void) {
    u64 rflags;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    return (rflags & (1 << 9)) != 0;
}
