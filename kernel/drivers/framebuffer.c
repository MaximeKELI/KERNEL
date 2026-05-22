#include "framebuffer.h"
#include "multiboot2_fb.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "font8x8.h"
#include "string.h"

#define FB_DEFAULT_W 320
#define FB_DEFAULT_H 200
#define FB_DEFAULT_BPP 32

static framebuffer_t fb = {0};
static bool fb_owned = false;
static u8 fb_type = 0; /* 0=none 1=software 2=multiboot */

static u32 fb_bytes_per_pixel(void) {
    return fb.bpp / 8;
}

static bool fb_resize(u32 w, u32 h, u32 bpp) {
    if (bpp < 8 || (bpp % 8) != 0) {
        return false;
    }
    u32 bpp8 = bpp / 8;
    if (w > 1920 || h > 1080) {
        return false;
    }
    if (w > UINT32_MAX / bpp8) {
        return false;
    }
    u32 pitch = w * bpp8;
    u64 size = (u64)pitch * h;
    if (size > 16 * 1024 * 1024) {
        return false;
    }

    if (fb_owned && fb.addr) {
        vmm_free_pages(fb.addr, (fb.size + PAGE_SIZE - 1) / PAGE_SIZE);
        fb.addr = NULL;
        fb_owned = false;
    }

    size_t pages = (size_t)((size + PAGE_SIZE - 1) / PAGE_SIZE);
    void* mem = vmm_alloc_pages(pages);
    if (!mem) {
        return false;
    }

    fb.addr = mem;
    fb.width = w;
    fb.height = h;
    fb.bpp = bpp;
    fb.pitch = pitch;
    fb.size = size;
    fb.enabled = true;
    fb_owned = true;
    fb_type = 1;
    memset(fb.addr, 0, (size_t)size);
    return true;
}

int framebuffer_init(u16 mode) {
    (void)mode;
    if (fb.enabled) {
        return 0;
    }
    if (!fb_resize(FB_DEFAULT_W, FB_DEFAULT_H, FB_DEFAULT_BPP)) {
        return -1;
    }
    printk("[FB] Software %ux%u@%ubpp (%zu KiB)\n",
           fb.width, fb.height, fb.bpp, (size_t)(fb.size / 1024));
    return 0;
}

int framebuffer_init_multiboot(u64 mb_info) {
    u64 addr = 0;
    u32 w = 0, h = 0, pitch = 0, bpp = 0;

    if (multiboot2_find_framebuffer(mb_info, &addr, &w, &h, &pitch, &bpp)) {
        if (fb_owned && fb.addr) {
            vmm_free_pages(fb.addr, (fb.size + PAGE_SIZE - 1) / PAGE_SIZE);
            fb_owned = false;
        }
        fb.addr = (void*)addr;
        fb.width = w;
        fb.height = h;
        fb.bpp = bpp;
        fb.pitch = pitch ? pitch : w * (bpp / 8);
        fb.size = (u64)fb.pitch * h;
        fb.enabled = true;
        fb_type = 2;
        printk("[FB] Multiboot2 %ux%u@%ubpp @0x%llx\n",
               w, h, bpp, (unsigned long long)addr);
        return 0;
    }
    return framebuffer_init(0);
}

framebuffer_t* framebuffer_get(void) {
    return fb.enabled ? &fb : NULL;
}

u8 framebuffer_get_type(void) {
    return fb_type;
}

void framebuffer_put_pixel(u32 x, u32 y, u32 color) {
    if (!fb.enabled || !fb.addr || x >= fb.width || y >= fb.height) {
        return;
    }
    u32 bpp8 = fb_bytes_per_pixel();
    u8* p = (u8*)fb.addr + y * fb.pitch + x * bpp8;
    if (bpp8 == 4) {
        *(u32*)p = color;
    } else if (bpp8 == 1) {
        *p = (u8)(color & 0xFF);
    }
}

void framebuffer_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    for (u32 py = y; py < y + h && py < fb.height; py++) {
        for (u32 px = x; px < x + w && px < fb.width; px++) {
            framebuffer_put_pixel(px, py, color);
        }
    }
}

void framebuffer_clear(u32 color) {
    if (!fb.enabled || !fb.addr) {
        return;
    }
    framebuffer_draw_rect(0, 0, fb.width, fb.height, color);
}

void framebuffer_draw_line(u32 x0, u32 y0, u32 x1, u32 y1, u32 color) {
    i64 dx = (i64)x1 - (i64)x0;
    i64 dy = (i64)y1 - (i64)y0;
    i64 steps = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx)
                                                         : (dy < 0 ? -dy : dy);
    if (steps == 0) {
        framebuffer_put_pixel(x0, y0, color);
        return;
    }
    for (i64 i = 0; i <= steps; i++) {
        u32 x = x0 + (u32)((dx * i) / steps);
        u32 y = y0 + (u32)((dy * i) / steps);
        framebuffer_put_pixel(x, y, color);
    }
}

void framebuffer_draw_char(u32 x, u32 y, char c, u32 fg, u32 bg) {
    const u8* glyph = font8x8_glyph(c);
    for (u32 row = 0; row < 8; row++) {
        u8 bits = glyph[row];
        for (u32 col = 0; col < 8; col++) {
            framebuffer_put_pixel(x + col, y + row, (bits & (1 << col)) ? fg : bg);
        }
    }
}

void framebuffer_draw_string(u32 x, u32 y, const char* s, u32 fg, u32 bg) {
    if (!s) {
        return;
    }
    u32 cx = x;
    while (*s) {
        framebuffer_draw_char(cx, y, *s++, fg, bg);
        cx += 8;
    }
}
