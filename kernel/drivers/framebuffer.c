#include "framebuffer.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "io.h"

static framebuffer_t fb = {0};

int framebuffer_init(u16 mode) {
    (void)mode;
    
    /* Would initialize VESA/VBE mode */
    /* For now, set up basic framebuffer */
    fb.width = 1024;
    fb.height = 768;
    fb.bpp = 32;
    fb.pitch = fb.width * 4;
    fb.size = fb.pitch * fb.height;
    fb.addr = (void*)0xE0000000; /* Placeholder address */
    fb.enabled = true;
    
    DEBUG_INFO("Framebuffer initialized: %ux%ux%u", fb.width, fb.height, fb.bpp);
    return 0;
}

framebuffer_t* framebuffer_get(void) {
    return fb.enabled ? &fb : NULL;
}

void framebuffer_put_pixel(u32 x, u32 y, u32 color) {
    if (!fb.enabled || x >= fb.width || y >= fb.height) return;
    
    u32* pixel = (u32*)((u8*)fb.addr + y * fb.pitch + x * 4);
    *pixel = color;
}

void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    for (u32 py = y; py < y + h && py < fb.height; py++) {
        for (u32 px = x; px < x + w && px < fb.width; px++) {
            framebuffer_put_pixel(px, py, color);
        }
    }
}

void framebuffer_clear(u32 color) {
    framebuffer_draw_rect(0, 0, fb.width, fb.height, color);
}

void framebuffer_draw_char(u32 x, u32 y, char c, u32 fg, u32 bg) {
    (void)x;
    (void)y;
    (void)c;
    (void)fg;
    (void)bg;
    /* Would draw character using font */
}
