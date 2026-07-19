#include "stdio.h"
#include "string.h"
#include "drivers/vga.h"
#include "serial.h"
#include <stdarg.h>

/* Simple printf implementation */
static void printk_putchar(char c) {
    if (serial_console_enabled) {
        serial_putchar(serial_console_port, c);
    }
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
                    print_number(val, 16, false);
                    written++;
                    break;
                }
                case 'X': {
                    u32 val = va_arg(args, u32);
                    print_number(val, 16, true);
                    written++;
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
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
                    print_number(val, 16, false);
                    written++;
                    break;
                }
                case 'X': {
                    u32 val = va_arg(args, u32);
                    print_number(val, 16, true);
                    written++;
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
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

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}

int vsnprintf(char* str, size_t size, const char* format, va_list args) {
    if (!str || size == 0) return 0;
    
    char* dest = str;
    size_t remaining = size - 1;
    const char* p = format;
    int written = 0;
    
    while (*p && remaining > 0) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    char num[32];
                    int i = 0;
                    if (val < 0) {
                        if (remaining > 0) {
                            *dest++ = '-';
                            remaining--;
                            written++;
                        }
                        val = -val;
                    }
                    if (val == 0) {
                        if (remaining > 0) {
                            *dest++ = '0';
                            remaining--;
                            written++;
                        }
                    } else {
                        while (val > 0 && i < 31) {
                            num[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                        while (i > 0 && remaining > 0) {
                            *dest++ = num[--i];
                            remaining--;
                            written++;
                        }
                    }
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s && remaining > 0) {
                        *dest++ = *s++;
                        remaining--;
                        written++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    if (remaining > 0) {
                        *dest++ = c;
                        remaining--;
                        written++;
                    }
                    break;
                }
                case 'x': {
                    u32 val = va_arg(args, u32);
                    char hex[] = "0123456789abcdef";
                    bool started = false;
                    for (int i = 28; i >= 0; i -= 4) {
                        u8 nibble = (val >> i) & 0xF;
                        if (nibble || started || i == 0) {
                            started = true;
                            if (remaining > 0) {
                                *dest++ = hex[nibble];
                                remaining--;
                                written++;
                            }
                        }
                    }
                    break;
                }
                default:
                    if (remaining > 0) {
                        *dest++ = '%';
                        *dest++ = *p;
                        remaining -= 2;
                        written += 2;
                    }
                    break;
            }
        } else {
            if (remaining > 0) {
                *dest++ = *p;
                remaining--;
                written++;
            }
        }
        p++;
    }
    
    *dest = '\0';
    return written;
}
