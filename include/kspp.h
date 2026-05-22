#ifndef KSPP_H
#define KSPP_H

#include "types.h"

/* Kernel Self Protection Project features */

/* Initialize KSPP */
void kspp_init(void);

/* Stack canary */
u64 stack_canary_get(void);
void stack_canary_set(u64 canary);

/* Address sanitizer support */
void* kspp_alloc(size_t size);
void kspp_free(void* ptr);

/* Control flow integrity */
void kspp_cfi_init(void);
bool kspp_cfi_check(void* target);

/* Read-only data protection */
int kspp_mark_rodata(void* addr, size_t size);
bool kspp_stack_check(void);

#endif /* KSPP_H */
