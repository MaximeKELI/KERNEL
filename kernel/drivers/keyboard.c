#include "drivers/keyboard.h"
#include "interrupt.h"
#include "io.h"
#include "stdio.h"

#include "hw_ports.h"

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
    
    u8 scancode = inb(KBD_DATA_PORT);
    
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
        __asm__ __volatile__("hlt");
    }
}

/*
 * Push a synthetic key press into the event queue. Used to bridge other input
 * sources (e.g. the serial console) and to drive the keyboard path from tests
 * without real hardware.
 */
void keyboard_inject_char(char c) {
    if (queue_size >= 256) {
        return;
    }
    keyboard_event_t event;
    event.scancode = 0;
    event.pressed = true;
    event.character = c;
    event_queue[queue_tail] = event;
    queue_tail = (queue_tail + 1) % 256;
    queue_size++;
}

/*
 * Blocking, line-buffered read (a minimal tty line discipline): echoes typed
 * characters, handles backspace, and returns once Enter is pressed or the
 * buffer is full. The returned length includes the trailing '\n' (if any) and
 * the buffer is NUL-terminated. This backs sys_read(stdin) so ring-3 programs
 * can read interactively.
 */
u32 keyboard_read_line(char* buf, u32 max) {
    u32 len = 0;
    if (!buf || max == 0) {
        return 0;
    }
    while (len + 1 < max) {
        char c = keyboard_read_char();
        if (c == '\b' || c == 127) {
            if (len > 0) {
                len--;
                printk("\b \b");
            }
            continue;
        }
        if (c == '\r' || c == '\n') {
            buf[len++] = '\n';
            printk("\n");
            break;
        }
        if (c >= 32 && c < 127) {
            buf[len++] = c;
            printk("%c", c);
        }
    }
    buf[len] = '\0';
    return len;
}
