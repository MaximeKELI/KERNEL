#include "cache.h"
#include "stdio.h"
#include "drivers/timer.h"

extern void ext2_journal_commit(void);

static u64 last_writeback_tick = 0;
#define WRITEBACK_INTERVAL_MS 5000

void writeback_init(void) {
    last_writeback_tick = timer_get_ticks();
    printk("[writeback] dirty page flush thread (timer-driven)\n");
}

void writeback_tick(void) {
    u64 now = timer_get_ticks();
    if (now - last_writeback_tick < WRITEBACK_INTERVAL_MS) {
        return;
    }
    last_writeback_tick = now;
    cache_sync_all();
    ext2_journal_commit();
}
