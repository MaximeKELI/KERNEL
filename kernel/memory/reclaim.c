#include "kswapd.h"
#include "memory.h"
#include "cache.h"
#include "stdio.h"
#include "debug.h"
#include "types.h"
#include "slab.h"
#include "memory_compaction.h"

static u64 memory_compact_reclaim(u64 count);

/* Reclaim pages from various sources */
u64 memory_reclaim_pages(u64 count) {
    u64 reclaimed = 0;

    reclaimed += cache_reclaim_pages(count / 2);
    reclaimed += slab_reclaim_pages(count / 2);

    if (reclaimed < count) {
        reclaimed += memory_compact_reclaim(count - reclaimed);
    }

    return reclaimed;
}

static u64 memory_compact_reclaim(u64 count) {
    if (count == 0) {
        return 0;
    }
    if (compact_memory(COMPACT_MODE_LIGHT) == 0) {
        return count > 4 ? 4 : count;
    }
    return 0;
}
