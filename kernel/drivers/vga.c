#include "drivers/vga.h"
#include "io.h"

#define VGA_MEMORY 0xB8000
#define VGA_INDEX_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

static u16* vga_buffer = (u16*)VGA_MEMORY;
static u8 vga_color = VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4);
static u8 cursor_x = 0;
static u8 cursor_y = 0;

static u8 vga_entry_color(u8 fg, u8 bg) {
    return fg | (bg << 4);
}

static u16 vga_entry(unsigned char uc, u8 color) {
    return (u16)uc | ((u16)color << 8);
}

static void vga_update_cursor(void) {
    u16 pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(VGA_INDEX_REGISTER, 0x0F);
    outb(VGA_DATA_REGISTER, (u8)(pos & 0xFF));
    outb(VGA_INDEX_REGISTER, 0x0E);
    outb(VGA_DATA_REGISTER, (u8)((pos >> 8) & 0xFF));
}

static void vga_scroll(void) {
    for (u32 i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (u32 i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_entry(' ', vga_color);
    }
    cursor_y = VGA_HEIGHT - 1;
}

void vga_init(void) {
    vga_clear();
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', vga_color);
        }
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, vga_color);
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
    
    vga_update_cursor();
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_clear(void) {
    for (u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_color);
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

void vga_set_color(u8 fg, u8 bg) {
    vga_color = vga_entry_color(fg, bg);
}

void vga_get_cursor(u8* x, u8* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void vga_set_cursor(u8 x, u8 y) {
    cursor_x = x;
    cursor_y = y;
    if (cursor_x >= VGA_WIDTH) cursor_x = VGA_WIDTH - 1;
    if (cursor_y >= VGA_HEIGHT) cursor_y = VGA_HEIGHT - 1;
    vga_update_cursor();
}
