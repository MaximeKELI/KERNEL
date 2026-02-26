#ifndef MEMORY_COMPACTION_H
#define MEMORY_COMPACTION_H

#include "types.h"

/* Compaction modes */
#define COMPACT_MODE_NONE    0
#define COMPACT_MODE_LIGHT   1
#define COMPACT_MODE_FULL    2
#define COMPACT_MODE_AGGRESSIVE 3

/* Compaction statistics */
typedef struct compact_stats {
    u64 pages_migrated;
    u64 pages_freed;
    u64 pages_scanned;
    u64 compaction_events;
} compact_stats_t;

/* Compact memory */
int compact_memory(u32 mode);

/* Get compaction statistics */
void compact_get_stats(compact_stats_t* stats);

/* Trigger compaction on memory pressure */
void compact_on_pressure(void);

#endif /* MEMORY_COMPACTION_H */
