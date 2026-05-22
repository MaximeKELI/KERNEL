#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"

#define VESA_MODE_640x480_8  0x101
#define VESA_MODE_800x600_8  0x103
#define VESA_MODE_1024x768_8 0x105
#define VESA_MODE_1280x1024_8 0x107

#define FB_COLOR_BLACK   0xFF000000
#define FB_COLOR_BLUE    0xFF0000AA
#define FB_COLOR_GREEN   0xFF00AA00
#define FB_COLOR_CYAN    0xFF00AAAA
#define FB_COLOR_RED     0xFFAA0000
#define FB_COLOR_MAGENTA 0xFFAA00AA
#define FB_COLOR_YELLOW  0xFFAAAA00
#define FB_COLOR_WHITE   0xFFFFFFFF

typedef struct framebuffer {
    void* addr;
    u32 width;
    u32 height;
    u32 bpp;
    u32 pitch;
    u64 size;
    bool enabled;
} framebuffer_t;

int framebuffer_init(u16 mode);
int framebuffer_init_multiboot(u64 mb_info);
framebuffer_t* framebuffer_get(void);
u8 framebuffer_get_type(void);

void framebuffer_put_pixel(u32 x, u32 y, u32 color);
void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
void framebuffer_draw_line(u32 x0, u32 y0, u32 x1, u32 y1, u32 color);
void framebuffer_clear(u32 color);
void framebuffer_draw_char(u32 x, u32 y, char c, u32 fg, u32 bg);
void framebuffer_draw_string(u32 x, u32 y, const char* s, u32 fg, u32 bg);

#endif /* FRAMEBUFFER_H */
