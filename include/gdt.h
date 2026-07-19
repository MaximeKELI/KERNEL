#ifndef GDT_H
#define GDT_H

#include "types.h"

/* Segment selectors (RPL=0 unless noted) */
#define GDT_KERNEL_CODE   0x08
#define GDT_KERNEL_DATA   0x10
#define GDT_USER_DATA     0x1B  /* index 3, DPL=3 */
#define GDT_USER_CODE     0x23  /* index 4, DPL=3 — SYSRET SS = CS+8 => 0x2B */
#define GDT_TSS_SEL       0x38  /* index 7 (+8 = 16-byte system descriptor) */

void gdt_init_user_segments(void);
void gdt_load(void);

/*
 * Write the 64-bit TSS system descriptor (16 bytes, occupying GDT indices 7-8)
 * for a TSS at `base` with byte limit `limit`. The GDT must already be loaded;
 * the CPU re-reads the descriptor from memory when ltr executes.
 */
void gdt_install_tss(u64 base, u32 limit);

#endif /* GDT_H */
