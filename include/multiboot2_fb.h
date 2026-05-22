#ifndef MULTIBOOT2_FB_H
#define MULTIBOOT2_FB_H

#include "types.h"

#define MULTIBOOT2_MAGIC 0x36d76289U
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

typedef struct {
    u32 type;
    u32 size;
    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u8 framebuffer_bpp;
    u8 framebuffer_type;
    u16 reserved;
} __attribute__((packed)) multiboot_tag_framebuffer_t;

bool multiboot2_find_framebuffer(u64 mb_info, u64* addr, u32* w, u32* h,
                                  u32* pitch, u32* bpp);

#endif /* MULTIBOOT2_FB_H */
