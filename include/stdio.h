#ifndef STDIO_H
#define STDIO_H

#include "types.h"

/* Kernel logging */
int printk(const char* format, ...);
void kprintf(const char* format, ...);

/* Debug output */
void debug_putchar(char c);
void debug_puts(const char* str);

#endif /* STDIO_H */
