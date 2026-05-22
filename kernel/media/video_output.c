#include "video_output.h"
#include "framebuffer.h"
#include "vga_graphics.h"
#include "video_core.h"
#include "drm.h"
#include "stdio.h"

static u8 rgb_to_vga8(u32 color) {
    u8 r = (color >> 16) & 0xFF;
    u8 g = (color >> 8) & 0xFF;
    u8 b = color & 0xFF;
    return (u8)(((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6));
}

void video_output_init(void) {
    vga_graphics_init();
}

void video_output_info(void) {
    framebuffer_t* f = framebuffer_get();
    if (!f) {
        printk("Video: framebuffer not ready (init-full)\n");
        return;
    }
    printk("\n=== Video ===\n");
    printk("FB: %ux%u %ubpp pitch=%u\n", f->width, f->height, f->bpp, f->pitch);
    printk("Type: %s\n",
           framebuffer_get_type() == 2 ? "multiboot" :
           framebuffer_get_type() == 1 ? "software" : "none");
    printk("VGA gfx: %s (320x200 mode 13h)\n",
           vga_graphics_active() ? "on" : "off");
    printk("DRM devices: %u\n", drm_get_device_count());
    video_device_t* dev = video_find_device("primary");
    if (dev) {
        printk("Device primary: %ux%u mode#%u\n",
               video_get_width(dev), video_get_height(dev),
               video_get_current_mode(dev));
    }
    printk("\n");
}

void video_output_demo(void) {
    framebuffer_t* f = framebuffer_get();
    if (!f) {
        printk("Framebuffer missing\n");
        return;
    }

    framebuffer_clear(FB_COLOR_BLACK);
    framebuffer_draw_rect(10, 10, f->width - 20, 30, FB_COLOR_BLUE);
    framebuffer_draw_string(20, 18, "KERNEL VIDEO", FB_COLOR_WHITE, FB_COLOR_BLUE);
    framebuffer_draw_line(0, 50, f->width - 1, 50, FB_COLOR_CYAN);
    framebuffer_draw_rect(40, 80, 120, 80, FB_COLOR_RED);
    framebuffer_draw_rect(180, 90, 100, 60, FB_COLOR_GREEN);
    framebuffer_draw_string(20, f->height - 24, "demo ok", FB_COLOR_YELLOW, FB_COLOR_BLACK);

    printk("[Video] Demo drawn on framebuffer\n");
}

void video_output_blit_vga(void) {
    framebuffer_t* f = framebuffer_get();
    if (!f || !f->addr) {
        printk("No framebuffer\n");
        return;
    }
    if (!vga_graphics_active()) {
        vga_graphics_init();
    }

    u32 w = f->width;
    u32 h = f->height;
    if (w > VGA_GFX_WIDTH) {
        w = VGA_GFX_WIDTH;
    }
    if (h > VGA_GFX_HEIGHT) {
        h = VGA_GFX_HEIGHT;
    }

    u32 bpp8 = f->bpp / 8;
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            u8* p = (u8*)f->addr + y * f->pitch + x * bpp8;
            u32 color = (bpp8 == 4) ? *(u32*)p : (*p);
            vga_graphics_plot(x, y, rgb_to_vga8(color));
        }
    }
    printk("[Video] Blit %ux%u -> VGA graphics\n", w, h);
}
