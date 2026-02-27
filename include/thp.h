#ifndef THP_H
#define THP_H

#include "types.h"

/* Initialize Transparent Huge Pages */
void thp_init(void);

/* Allocate memory with THP (automatic huge page promotion) */
void* thp_alloc(size_t size);

/* Free THP memory */
void thp_free(void* addr, size_t size);

/* Enable/disable THP */
void thp_enable(void);
void thp_disable(void);
bool thp_is_enabled(void);

/* Defragmentation control */
void thp_set_defrag(bool enable);
bool thp_get_defrag(void);

/* Get THP statistics */
void thp_get_stats(u64* allocations, u64* failures);

#endif /* THP_H */
