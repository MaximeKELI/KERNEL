#ifndef STDIO_H
#define STDIO_H

#include "types.h"
#include <stdarg.h>

/* Kernel logging */
int printk(const char* format, ...);
void kprintf(const char* format, ...);

/* Debug output */
void debug_putchar(char c);
void debug_puts(const char* str);

/* Formatted output to buffer */
int snprintf(char* str, size_t size, const char* format, ...);
int vsnprintf(char* str, size_t size, const char* format, va_list args);

#endif /* STDIO_H */
