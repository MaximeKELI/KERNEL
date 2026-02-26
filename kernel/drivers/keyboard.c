#include "drivers/keyboard.h"
#include "interrupt.h"
#include "io.h"
#include "stdio.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static keyboard_event_t event_queue[256];
static u32 queue_head = 0;
static u32 queue_tail = 0;
static u32 queue_size = 0;

/* Scancode to ASCII mapping */
static char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void keyboard_irq_handler(u32 irq, void* data) {
    (void)irq;
    (void)data;
    
    u8 scancode = inb(KEYBOARD_DATA_PORT);
    
    if (queue_size < 256) {
        keyboard_event_t event;
        event.scancode = scancode;
        event.pressed = !(scancode & 0x80);
        event.character = (scancode < 128 && event.pressed) ? scancode_to_ascii[scancode] : 0;
        
        event_queue[queue_tail] = event;
        queue_tail = (queue_tail + 1) % 256;
        queue_size++;
    }
}

void keyboard_init(void) {
    irq_register(1, keyboard_irq_handler, NULL);
    printk("Keyboard: Initialized\n");
}

bool keyboard_get_event(keyboard_event_t* event) {
    if (queue_size == 0) return false;
    
    *event = event_queue[queue_head];
    queue_head = (queue_head + 1) % 256;
    queue_size--;
    return true;
}

char keyboard_read_char(void) {
    keyboard_event_t event;
    while (true) {
        if (keyboard_get_event(&event) && event.pressed && event.character) {
            return event.character;
        }
        /* Wait for interrupt */
        asm volatile("hlt");
    }
}
