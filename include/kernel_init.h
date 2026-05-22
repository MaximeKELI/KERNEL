#ifndef KERNEL_INIT_H
#define KERNEL_INIT_H

#include "types.h"

/* Fast path: core subsystems only (QEMU-friendly) */
void kernel_init_minimal(void);

/* Full subsystem init (optional, via shell command 'init-full') */
void kernel_init_extended(void);

bool kernel_extended_ready(void);

#endif /* KERNEL_INIT_H */
