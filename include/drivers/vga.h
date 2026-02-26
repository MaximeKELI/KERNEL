#ifndef VGA_H
#define VGA_H

#include "types.h"

/* VGA colors */
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_WHITE         15

/* VGA dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Initialize VGA */
void vga_init(void);

/* Put character */
void vga_putchar(char c);

/* Put string */
void vga_puts(const char* str);

/* Clear screen */
void vga_clear(void);

/* Set color */
void vga_set_color(u8 fg, u8 bg);

/* Get current position */
void vga_get_cursor(u8* x, u8* y);

/* Set cursor position */
void vga_set_cursor(u8 x, u8 y);

#endif /* VGA_H */
