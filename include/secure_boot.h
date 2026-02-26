#ifndef SECURE_BOOT_H
#define SECURE_BOOT_H

#include "types.h"

/* Secure boot states */
#define SECURE_BOOT_DISABLED 0
#define SECURE_BOOT_ENABLED  1
#define SECURE_BOOT_LOCKED   2

/* Initialize secure boot */
void secure_boot_init(void);

/* Verify kernel signature */
int secure_boot_verify_kernel(void* kernel_image, size_t size);

/* Verify module signature */
int secure_boot_verify_module(void* module_image, size_t size);

/* Get secure boot state */
u32 secure_boot_get_state(void);

/* Lock secure boot */
int secure_boot_lock(void);

#endif /* SECURE_BOOT_H */
