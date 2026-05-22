#include "serial.h"
#include "io.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"

#define SERIAL_DATA_PORT(port)     ((port) + UART_REG_RBR)
#define SERIAL_FIFO_PORT(port)     ((port) + UART_REG_IIR_FCR)
#define SERIAL_LINE_PORT(port)     ((port) + UART_REG_LCR)
#define SERIAL_MODEM_PORT(port)    ((port) + UART_REG_MCR)
#define SERIAL_LINE_STATUS(port)   ((port) + UART_REG_LSR)

typedef struct {
    u16 base;
    const char* name;
    bool initialized;
} serial_port_entry_t;

static serial_port_entry_t serial_ports[SERIAL_PORT_COUNT] = {
    {COM1, "COM1", false},
    {COM2, "COM2", false},
    {COM3, "COM3", false},
    {COM4, "COM4", false},
};

bool serial_console_enabled = false;
u16 serial_console_port = COM1;

static serial_port_entry_t* serial_find_entry(u16 port) {
    for (u32 i = 0; i < SERIAL_PORT_COUNT; i++) {
        if (serial_ports[i].base == port) {
            return &serial_ports[i];
        }
    }
    return NULL;
}

bool serial_port_valid(u16 port) {
    return serial_find_entry(port) != NULL;
}

const char* serial_port_name(u16 port) {
    serial_port_entry_t* e = serial_find_entry(port);
    return e ? e->name : NULL;
}

u16 serial_port_by_name(const char* name) {
    if (!name) {
        return 0;
    }
    for (u32 i = 0; i < SERIAL_PORT_COUNT; i++) {
        if (strcmp(serial_ports[i].name, name) == 0) {
            return serial_ports[i].base;
        }
    }
    return 0;
}

bool serial_port_is_init(u16 port) {
    serial_port_entry_t* e = serial_find_entry(port);
    return e && e->initialized;
}

void serial_init(u16 port) {
    serial_port_entry_t* e = serial_find_entry(port);
    if (!e) {
        DEBUG_ERROR("Invalid serial port 0x%x", port);
        return;
    }

    outb(port + 1, 0x00);
    outb(SERIAL_LINE_PORT(port), 0x80);
    outb(SERIAL_DATA_PORT(port), 0x01);
    outb(SERIAL_DATA_PORT(port) + 1, 0x00);
    outb(SERIAL_LINE_PORT(port), 0x03);
    outb(SERIAL_FIFO_PORT(port), 0xC7);
    outb(SERIAL_MODEM_PORT(port), 0x0B);

    e->initialized = true;
    serial_console_port = port;
    serial_console_enabled = true;
    DEBUG_INFO("Serial %s (0x%x) initialized", e->name, port);
}

void serial_init_all(void) {
    for (u32 i = 0; i < SERIAL_PORT_COUNT; i++) {
        serial_init(serial_ports[i].base);
    }
    printk("[Serial] COM1-COM4 initialized\n");
}

void serial_putchar(u16 port, char c) {
    if (!serial_port_valid(port)) {
        return;
    }
    while (!(inb(SERIAL_LINE_STATUS(port)) & 0x20)) {
        /* wait */
    }
    outb(port, c);
}

char serial_getchar(u16 port) {
    while (!(inb(SERIAL_LINE_STATUS(port)) & 0x01)) {
        /* wait */
    }
    return inb(port);
}

void serial_puts(u16 port, const char* str) {
    if (!str) {
        return;
    }
    while (*str) {
        serial_putchar(port, *str++);
    }
}

bool serial_data_available(u16 port) {
    return (inb(SERIAL_LINE_STATUS(port)) & 0x01) != 0;
}
