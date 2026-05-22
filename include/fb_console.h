#ifndef FB_CONSOLE_H
#define FB_CONSOLE_H

#include "types.h"

void fb_console_init(void);
void fb_console_write(const char* s, u64 len);
void fb_console_putchar(char c);

#endif /* FB_CONSOLE_H */
