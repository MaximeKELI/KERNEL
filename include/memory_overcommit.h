#ifndef MEMORY_OVERCOMMIT_H
#define MEMORY_OVERCOMMIT_H

#include "types.h"

/* Overcommit modes */
#define MEMORY_OVERCOMMIT_NONE    0
#define MEMORY_OVERCOMMIT_ALWAYS  1
#define MEMORY_OVERCOMMIT_GUESS   2

/* Initialize memory overcommit system */
void memory_overcommit_init(void);

/* Check if overcommit is allowed for requested pages */
bool memory_overcommit_allowed(size_t requested_pages);

/* Set/get overcommit mode */
void memory_overcommit_set_mode(u32 mode);
u32 memory_overcommit_get_mode(void);

/* Set/get overcommit ratio (0-100%) */
void memory_overcommit_set_ratio(u64 ratio);
u64 memory_overcommit_get_ratio(void);

/* Get overcommit statistics */
void memory_overcommit_get_stats(u64* allocations, u64* failures);

#endif /* MEMORY_OVERCOMMIT_H */
