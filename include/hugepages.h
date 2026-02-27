#ifndef HUGEPAGES_H
#define HUGEPAGES_H

#include "types.h"

/* Huge page sizes */
#define HUGEPAGE_2MB (2 * 1024 * 1024)
#define HUGEPAGE_1GB (1024 * 1024 * 1024)

/* Initialize huge pages */
void hugepages_init(void);

/* Allocate huge page */
void* hugepage_alloc(size_t size);

/* Free huge page */
void hugepage_free(void* addr, size_t size);

/* Get huge page statistics */
void hugepages_get_stats(u64* total, u64* free, u64* used);

/* Check if address is a huge page */
bool hugepage_is_huge(void* addr);

#endif /* HUGEPAGES_H */
