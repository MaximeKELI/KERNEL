#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

/* PC UART base addresses */
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

#define SERIAL_PORT_COUNT 4

extern bool serial_console_enabled;
extern u16 serial_console_port;

bool serial_port_valid(u16 port);
const char* serial_port_name(u16 port);
u16 serial_port_by_name(const char* name);
bool serial_port_is_init(u16 port);
void serial_init(u16 port);
void serial_init_all(void);

/* Write character */
void serial_putchar(u16 port, char c);

/* Read character */
char serial_getchar(u16 port);

/* Write string */
void serial_puts(u16 port, const char* str);

/* Check if data available */
bool serial_data_available(u16 port);

#endif /* SERIAL_H */
