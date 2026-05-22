#ifndef KASLR_H
#define KASLR_H

#include "types.h"

/* Initialize KASLR */
void kaslr_init(void);

/* Get random address offset */
u64 kaslr_get_offset(void);

/* Randomize kernel base */
void kaslr_randomize_kernel_base(void);

/* Randomize module base */
u64 kaslr_randomize_module_base(void);

/* Get ASLR status */
bool kaslr_enabled(void);

/* Alias used by tests */
static inline bool kaslr_is_enabled(void) {
    return kaslr_enabled();
}

#endif /* KASLR_H */
