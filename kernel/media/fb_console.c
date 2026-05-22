#include "fb_console.h"
#include "framebuffer.h"
#include "font8x8.h"
#include "string.h"

#define FB_CON_COLS 80
#define FB_CON_ROWS 25

static u32 con_x = 0;
static u32 con_y = 0;

void fb_console_init(void) {
    con_x = 0;
    con_y = 0;
}

void fb_console_putchar(char c) {
    framebuffer_t* fb = framebuffer_get();
    if (!fb || !fb->buffer) {
        return;
    }

    if (c == '\n') {
        con_x = 0;
        con_y++;
        if (con_y >= FB_CON_ROWS) {
            con_y = FB_CON_ROWS - 1;
        }
        return;
    }

    u32 px = con_x * 8;
    u32 py = con_y * 8;
    if (px + 8 < fb->width && py + 8 < fb->height) {
        font8x8_draw_char(fb->buffer, fb->width, fb->height, px, py, (u8)c, 0x00FFFFFF, 0x00000000);
    }
    con_x++;
    if (con_x >= FB_CON_COLS) {
        con_x = 0;
        con_y++;
    }
}

void fb_console_write(const char* s, u64 len) {
    if (!s) {
        return;
    }
    for (u64 i = 0; i < len; i++) {
        fb_console_putchar(s[i]);
    }
}
