#include "vga_graphics.h"
#include "hw_ports.h"
#include "stdio.h"

static bool gfx_on = false;

static void write_crtc(u8 idx, u8 val) {
    outb(VGA_CRTC_INDEX, idx);
    outb(VGA_CRTC_DATA, val);
}

void vga_graphics_init(void) {
    /* Mode 13h: 320x200x256 @ 0xA0000 */
    outb(0x3C2, 0x63);
    write_crtc(0x11, 0x00);

    outb(0x3C0, 0x00);
    outb(0x3C0, 0x20);

    write_crtc(0x00, 0x5F);
    write_crtc(0x01, 0x4F);
    write_crtc(0x02, 0x50);
    write_crtc(0x03, 0x82);
    write_crtc(0x04, 0x54);
    write_crtc(0x05, 0x80);
    write_crtc(0x06, 0xBF);
    write_crtc(0x07, 0x1F);
    write_crtc(0x08, 0x00);
    write_crtc(0x09, 0x41);
    write_crtc(0x0A, 0x00);
    write_crtc(0x0B, 0x00);
    write_crtc(0x0C, 0x00);
    write_crtc(0x0D, 0x00);
    write_crtc(0x0E, 0x00);
    write_crtc(0x0F, 0x00);
    write_crtc(0x10, 0x9C);
    write_crtc(0x11, 0x8E);
    write_crtc(0x12, 0x8F);
    write_crtc(0x13, 0x28);
    write_crtc(0x14, 0x40);
    write_crtc(0x15, 0x96);
    write_crtc(0x16, 0xB9);
    write_crtc(0x17, 0xA3);

    outb(0x3C4, 0x00);
    outb(0x3C4, 0x01);
    outb(0x3C4, 0x02);
    outb(0x3C4, 0x03);
    outb(0x3C4, 0x04);

    outb(0x3D4, 0x13);
    outb(0x3D5, 0x00);

    gfx_on = true;
    vga_graphics_clear(0);
    printk("[VGA] Graphics mode 320x200x8 (0xA0000)\n");
}

bool vga_graphics_active(void) {
    return gfx_on;
}

void vga_graphics_plot(u32 x, u32 y, u8 color) {
    if (!gfx_on || x >= VGA_GFX_WIDTH || y >= VGA_GFX_HEIGHT) {
        return;
    }
    volatile u8* fb = (volatile u8*)(VGA_GFX_FB + y * VGA_GFX_WIDTH + x);
    *fb = color;
}

void vga_graphics_clear(u8 color) {
    if (!gfx_on) {
        return;
    }
    for (u32 y = 0; y < VGA_GFX_HEIGHT; y++) {
        for (u32 x = 0; x < VGA_GFX_WIDTH; x++) {
            vga_graphics_plot(x, y, color);
        }
    }
}

void vga_graphics_rect(u32 x, u32 y, u32 w, u32 h, u8 color) {
    for (u32 py = y; py < y + h && py < VGA_GFX_HEIGHT; py++) {
        for (u32 px = x; px < x + w && px < VGA_GFX_WIDTH; px++) {
            vga_graphics_plot(px, py, color);
        }
    }
}
