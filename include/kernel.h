#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

/* Kernel version */
#define KERNEL_VERSION_MAJOR 1
#define KERNEL_VERSION_MINOR 0
#define KERNEL_VERSION_PATCH 0

/* Kernel entry point */
void kernel_main(u64 magic, u64 mb_info);

/* Panic handler */
__noreturn void panic(const char* message);

/* Halt CPU */
__noreturn void halt(void);

#endif /* KERNEL_H */
