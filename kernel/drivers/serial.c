#include "serial.h"
#include "io.h"
#include "stdio.h"
#include "debug.h"

#define SERIAL_DATA_PORT(port)     (port)
#define SERIAL_FIFO_PORT(port)     (port + 2)
#define SERIAL_LINE_PORT(port)     (port + 3)
#define SERIAL_MODEM_PORT(port)    (port + 4)
#define SERIAL_LINE_STATUS(port)   (port + 5)

bool serial_console_enabled = false;
u16 serial_console_port = COM1;

void serial_init(u16 port) {
    /* Disable interrupts */
    outb(port + 1, 0x00);
    
    /* Enable DLAB */
    outb(SERIAL_LINE_PORT(port), 0x80);
    
    /* Set baud rate (115200) */
    outb(SERIAL_DATA_PORT(port), 0x01);
    outb(SERIAL_DATA_PORT(port) + 1, 0x00);
    
    /* 8 bits, no parity, 1 stop bit */
    outb(SERIAL_LINE_PORT(port), 0x03);
    
    /* Enable FIFO, clear, 14 bytes threshold */
    outb(SERIAL_FIFO_PORT(port), 0xC7);
    
    /* IRQs enabled, RTS/DSR set */
    outb(SERIAL_MODEM_PORT(port), 0x0B);
    
    serial_console_port = port;
    serial_console_enabled = true;
    DEBUG_INFO("Serial port 0x%x initialized", port);
}

void serial_putchar(u16 port, char c) {
    /* Wait for transmit buffer empty */
    while (!(inb(SERIAL_LINE_STATUS(port)) & 0x20));
    outb(port, c);
}

char serial_getchar(u16 port) {
    /* Wait for data available */
    while (!(inb(SERIAL_LINE_STATUS(port)) & 0x01));
    return inb(port);
}

void serial_puts(u16 port, const char* str) {
    while (*str) {
        serial_putchar(port, *str++);
    }
}

bool serial_data_available(u16 port) {
    return (inb(SERIAL_LINE_STATUS(port)) & 0x01) != 0;
}
