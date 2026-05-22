#ifndef VGA_GRAPHICS_H
#define VGA_GRAPHICS_H

#include "types.h"

#define VGA_GFX_WIDTH  320
#define VGA_GFX_HEIGHT 200
#define VGA_GFX_FB     0xA0000UL

void vga_graphics_init(void);
bool vga_graphics_active(void);
void vga_graphics_plot(u32 x, u32 y, u8 color);
void vga_graphics_clear(u8 color);
void vga_graphics_rect(u32 x, u32 y, u32 w, u32 h, u8 color);

#endif /* VGA_GRAPHICS_H */
