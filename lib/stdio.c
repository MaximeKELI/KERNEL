#include "stdio.h"
#include "string.h"
#include "drivers/vga.h"
#include <stdarg.h>

/* Simple printf implementation */
static void printk_putchar(char c) {
    vga_putchar(c);
}

static void print_number(u64 num, int base, bool uppercase) {
    char digits[] = "0123456789abcdef0123456789ABCDEF";
    char buffer[64];
    int i = 0;

    if (num == 0) {
        printk_putchar('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = digits[(num % base) + (uppercase ? 16 : 0)];
        num /= base;
    }

    while (i > 0) {
        printk_putchar(buffer[--i]);
    }
}

int printk(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int written = 0;
    const char* p = format;

    while (*p) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    if (val < 0) {
                        printk_putchar('-');
                        val = -val;
                    }
                    print_number(val, 10, false);
                    written++;
                    break;
                }
                case 'u': {
                    u32 val = va_arg(args, u32);
                    print_number(val, 10, false);
                    written++;
                    break;
                }
                case 'x': {
                    u32 val = va_arg(args, u32);
                    printk_putchar('0');
                    printk_putchar('x');
                    print_number(val, 16, false);
                    written++;
                    break;
                }
                case 'X': {
                    u32 val = va_arg(args, u32);
                    printk_putchar('0');
                    printk_putchar('X');
                    print_number(val, 16, true);
                    written++;
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    printk_putchar('0');
                    printk_putchar('x');
                    print_number((u64)ptr, 16, false);
                    written++;
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str) {
                        printk_putchar(*str++);
                        written++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    printk_putchar(c);
                    written++;
                    break;
                }
                case '%': {
                    printk_putchar('%');
                    written++;
                    break;
                }
                default:
                    printk_putchar('%');
                    printk_putchar(*p);
                    written += 2;
                    break;
            }
        } else {
            printk_putchar(*p);
            written++;
        }
        p++;
    }

    va_end(args);
    return written;
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int written = 0;
    const char* p = format;

    while (*p) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    if (val < 0) {
                        vga_putchar('-');
                        val = -val;
                    }
                    print_number(val, 10, false);
                    written++;
                    break;
                }
                case 'u': {
                    u32 val = va_arg(args, u32);
                    print_number(val, 10, false);
                    written++;
                    break;
                }
                case 'x': {
                    u32 val = va_arg(args, u32);
                    vga_putchar('0');
                    vga_putchar('x');
                    print_number(val, 16, false);
                    written++;
                    break;
                }
                case 'X': {
                    u32 val = va_arg(args, u32);
                    vga_putchar('0');
                    vga_putchar('X');
                    print_number(val, 16, true);
                    written++;
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    vga_putchar('0');
                    vga_putchar('x');
                    print_number((u64)ptr, 16, false);
                    written++;
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str) {
                        vga_putchar(*str++);
                        written++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putchar(c);
                    written++;
                    break;
                }
                case '%': {
                    vga_putchar('%');
                    written++;
                    break;
                }
                default:
                    vga_putchar('%');
                    vga_putchar(*p);
                    written += 2;
                    break;
            }
        } else {
            vga_putchar(*p);
            written++;
        }
        p++;
    }
    
    va_end(args);
}

void debug_putchar(char c) {
    printk_putchar(c);
}

void debug_puts(const char* str) {
    while (*str) {
        debug_putchar(*str++);
    }
}
