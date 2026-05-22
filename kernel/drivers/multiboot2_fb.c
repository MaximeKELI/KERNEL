#include "multiboot2_fb.h"

#define MULTIBOOT_TAG_ALIGN 8

bool multiboot2_find_framebuffer(u64 mb_info, u64* addr, u32* w, u32* h,
                                  u32* pitch, u32* bpp) {
    if (!mb_info) {
        return false;
    }

    u32 total = *(u32*)(mb_info);
    u32 off = 8;

    while (off + sizeof(u32) * 2 < total) {
        u32 type = *(u32*)(mb_info + off);
        u32 size = *(u32*)(mb_info + off + 4);

        if (size < 8 || off + size > total) {
            break;
        }

        if (type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER &&
            size >= sizeof(multiboot_tag_framebuffer_t)) {
            multiboot_tag_framebuffer_t* tag =
                (multiboot_tag_framebuffer_t*)(mb_info + off);
            if (addr) {
                *addr = tag->framebuffer_addr;
            }
            if (w) {
                *w = tag->framebuffer_width;
            }
            if (h) {
                *h = tag->framebuffer_height;
            }
            if (pitch) {
                *pitch = tag->framebuffer_pitch;
            }
            if (bpp) {
                *bpp = tag->framebuffer_bpp;
            }
            return tag->framebuffer_addr != 0;
        }

        off += (size + MULTIBOOT_TAG_ALIGN - 1) & ~(MULTIBOOT_TAG_ALIGN - 1);
    }
    return false;
}
