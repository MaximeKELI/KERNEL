#include "gdt.h"
#include "stdio.h"
#include "string.h"

typedef struct __attribute__((packed)) {
    u16 limit;
    u64 base;
} gdt_ptr_t;

/*
 * GDT layout (64-bit):
 * 0 null | 1 kernel code 0x08 | 2 kernel data 0x10
 * 3 user data 0x1B | 4 user code 0x23 | 5 user data2 0x2B (SYSRET SS = CS+8)
 * 6 unused | 7-8 TSS descriptor 0x38 (16-byte system segment)
 */
static u64 gdt[9];
static gdt_ptr_t gdt_desc;
static bool gdt_user_ready = false;

void gdt_init_user_segments(void) {
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFULL; /* kernel code 64-bit */
    gdt[2] = 0x00AF92000000FFFFULL; /* kernel data */
    gdt[3] = 0x00AFF2000000FFFFULL; /* user data DPL=3 */
    gdt[4] = 0x00AFFB000000FFFFULL; /* user code DPL=3, exec/read */
    gdt[5] = 0x00AFF2000000FFFFULL; /* user data @ 0x2B for SYSRET SS=CS+8 */
    gdt[6] = 0;

    gdt_desc.limit = (u16)(sizeof(gdt) - 1);
    gdt_desc.base = (u64)(uintptr_t)gdt;

    __asm__ volatile("lgdt %0" : : "m"(gdt_desc) : "memory");
    gdt_user_ready = true;
    printk("[GDT] user segments loaded (ring 3 ready)\n");
}

void gdt_load(void) {
    if (!gdt_user_ready) {
        gdt_init_user_segments();
    } else {
        __asm__ volatile("lgdt %0" : : "m"(gdt_desc) : "memory");
    }
}

void gdt_install_tss(u64 base, u32 limit) {
    /*
     * 64-bit TSS (available) system descriptor, type = 0x9, present, DPL 0.
     * It spans two 8-byte GDT slots: the low half is the classic segment
     * descriptor, the high half carries base bits 63:32.
     */
    u64 low = 0;
    low |= (u64)(limit & 0xFFFFu);
    low |= (base & 0xFFFFull) << 16;
    low |= ((base >> 16) & 0xFFull) << 32;
    low |= (u64)0x89ull << 40;                       /* P=1, DPL=0, type=9 (TSS avail) */
    low |= (u64)((limit >> 16) & 0xFu) << 48;        /* limit 19:16, flags=0 */
    low |= ((base >> 24) & 0xFFull) << 56;

    gdt[7] = low;
    gdt[8] = (base >> 32) & 0xFFFFFFFFull;
}
