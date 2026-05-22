#include "kswapd.h"
#include "memory.h"
#include "memory_pressure.h"
#include "kthread.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

static bool kswapd_running = false;
static kthread_t* kswapd_thread = NULL;
static spinlock_t kswapd_lock = SPINLOCK_INIT;

/* Reclaim statistics */
static u64 pages_reclaimed = 0;
static u64 reclaim_cycles = 0;

/* Reclaim thresholds */
#define RECLAIM_BATCH_SIZE 32

static void kswapd_worker(void* arg) {
    (void)arg;

    printk("[kswapd] Started\n");

    while (kswapd_running) {
        u32 pressure = memory_pressure_check();

        if (pressure >= MEMORY_PRESSURE_HIGH) {
            u64 reclaimed = memory_reclaim_pages(RECLAIM_BATCH_SIZE * 4);
            pages_reclaimed += reclaimed;
            reclaim_cycles++;
            if (reclaimed > 0) {
                DEBUG_INFO("kswapd: Reclaimed %u pages", (u32)reclaimed);
            }
        } else if (pressure >= MEMORY_PRESSURE_MEDIUM) {
            u64 reclaimed = memory_reclaim_pages(RECLAIM_BATCH_SIZE * 2);
            pages_reclaimed += reclaimed;
            reclaim_cycles++;
        }

        kthread_yield();
    }

    printk("[kswapd] Stopped\n");
}

void kswapd_init(void) {
    spinlock_lock(&kswapd_lock);

    if (kswapd_running) {
        spinlock_unlock(&kswapd_lock);
        return;
    }

    kswapd_running = true;
    pages_reclaimed = 0;
    reclaim_cycles = 0;

    kswapd_thread = kthread_create(kswapd_worker, NULL, "kswapd");
    if (!kswapd_thread) {
        DEBUG_ERROR("%s", "Failed to create kswapd thread");
        kswapd_running = false;
    } else {
        kthread_start(kswapd_thread);
        printk("[kswapd] Initialized\n");
    }

    spinlock_unlock(&kswapd_lock);
}

void kswapd_stop(void) {
    spinlock_lock(&kswapd_lock);

    if (!kswapd_running) {
        spinlock_unlock(&kswapd_lock);
        return;
    }

    kswapd_running = false;
    spinlock_unlock(&kswapd_lock);

    if (kswapd_thread) {
        kthread_stop(kswapd_thread);
        kswapd_thread = NULL;
    }
}

u64 kswapd_get_reclaimed_pages(void) {
    spinlock_lock(&kswapd_lock);
    u64 reclaimed = pages_reclaimed;
    spinlock_unlock(&kswapd_lock);
    return reclaimed;
}

u64 kswapd_get_reclaim_cycles(void) {
    spinlock_lock(&kswapd_lock);
    u64 cycles = reclaim_cycles;
    spinlock_unlock(&kswapd_lock);
    return cycles;
}

bool kswapd_is_running(void) {
    spinlock_lock(&kswapd_lock);
    bool running = kswapd_running;
    spinlock_unlock(&kswapd_lock);
    return running;
}
