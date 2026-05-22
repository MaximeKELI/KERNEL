#ifndef GDT_H
#define GDT_H

#include "types.h"

/* Segment selectors (RPL=0 unless noted) */
#define GDT_KERNEL_CODE   0x08
#define GDT_KERNEL_DATA   0x10
#define GDT_USER_DATA     0x1B  /* index 3, DPL=3 */
#define GDT_USER_CODE     0x23  /* index 4, DPL=3 — SYSRET SS = CS+8 => 0x2B */

void gdt_init_user_segments(void);
void gdt_load(void);

#endif /* GDT_H */
