#ifndef KASAN_H
#define KASAN_H

#include "types.h"

/* Initialize KASAN */
void kasan_init(void);

/* Enable KASAN */
void kasan_enable(void);

/* Disable KASAN */
void kasan_disable(void);

/* Mark memory as accessible */
void kasan_unpoison(void* addr, size_t size);

/* Mark memory as inaccessible */
void kasan_poison(void* addr, size_t size);

/* Check if address is valid */
bool kasan_check(void* addr, size_t size);

/* Report error */
void kasan_report(void* addr, size_t size, bool is_write);

#endif /* KASAN_H */
