#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"

/* VESA modes */
#define VESA_MODE_640x480_8  0x101
#define VESA_MODE_800x600_8  0x103
#define VESA_MODE_1024x768_8 0x105
#define VESA_MODE_1280x1024_8 0x107

/* Framebuffer structure */
typedef struct framebuffer {
    void* addr;
    u32 width;
    u32 height;
    u32 bpp;
    u32 pitch;
    u32 size;
    bool enabled;
} framebuffer_t;

/* Initialize framebuffer */
int framebuffer_init(u16 mode);

/* Get framebuffer */
framebuffer_t* framebuffer_get(void);

/* Draw pixel */
void framebuffer_put_pixel(u32 x, u32 y, u32 color);

/* Draw rectangle */
void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color);

/* Clear screen */
void framebuffer_clear(u32 color);

/* Draw character */
void framebuffer_draw_char(u32 x, u32 y, char c, u32 fg, u32 bg);

#endif /* FRAMEBUFFER_H */
