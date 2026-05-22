#include "kswapd.h"
#include "memory.h"
#include "cache.h"
#include "stdio.h"
#include "debug.h"
#include "types.h"

static u64 cache_reclaim_pages(u64 count);
static u64 slab_reclaim_pages(u64 count);
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

static u64 cache_reclaim_pages(u64 count) {
    (void)count;
    return 0;
}

static u64 slab_reclaim_pages(u64 count) {
    (void)count;
    return 0;
}

static u64 memory_compact_reclaim(u64 count) {
    (void)count;
    return 0;
}
