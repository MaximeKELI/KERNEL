#include "video_shell.h"
#include "video_output.h"
#include "framebuffer.h"
#include "vga_graphics.h"
#include "video_core.h"
#include "kernel_init.h"
#include "stdio.h"
#include "string.h"

void video_shell_command(const char* args) {
    if (!kernel_extended_ready()) {
        printk("Video: need init-full\n");
        return;
    }

    if (!args || !args[0]) {
        video_output_info();
        return;
    }

    if (strcmp(args, "info") == 0) {
        video_output_info();
    } else if (strcmp(args, "demo") == 0) {
        video_output_demo();
    } else if (strcmp(args, "blit") == 0) {
        video_output_blit_vga();
    } else if (strcmp(args, "clear") == 0) {
        framebuffer_clear(FB_COLOR_BLACK);
        if (vga_graphics_active()) {
            vga_graphics_clear(0);
        }
        printk("Video cleared\n");
    } else if (strcmp(args, "gfx") == 0) {
        vga_graphics_init();
    } else if (strcmp(args, "help") == 0) {
        printk("Video commands:\n");
        printk("  video           - info\n");
        printk("  video demo      - draw test pattern\n");
        printk("  video blit      - FB -> VGA 320x200\n");
        printk("  video clear     - clear FB+VGA\n");
        printk("  video gfx       - VGA mode 13h\n");
    } else {
        printk("Unknown: video %s (try: video help)\n", args);
    }
}
