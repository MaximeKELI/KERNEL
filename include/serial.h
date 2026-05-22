#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

/* Serial ports */
#define COM1 0x3F8

extern bool serial_console_enabled;
extern u16 serial_console_port;
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

/* Initialize serial port */
void serial_init(u16 port);

/* Write character */
void serial_putchar(u16 port, char c);

/* Read character */
char serial_getchar(u16 port);

/* Write string */
void serial_puts(u16 port, const char* str);

/* Check if data available */
bool serial_data_available(u16 port);

#endif /* SERIAL_H */
