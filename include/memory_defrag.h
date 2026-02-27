#ifndef MEMORY_DEFRAG_H
#define MEMORY_DEFRAG_H

#include "types.h"

/* Initialize memory defragmentation */
void memory_defrag_init(void);

/* Defragment a memory region */
u64 memory_defrag_region(void* start, size_t size);

/* Defragment entire system */
u64 memory_defrag_system(void);

/* Enable/disable defragmentation */
void memory_defrag_enable(void);
void memory_defrag_disable(void);
bool memory_defrag_is_enabled(void);

/* Get defragmentation statistics */
void memory_defrag_get_stats(u64* operations, u64* pages_moved);

#endif /* MEMORY_DEFRAG_H */
