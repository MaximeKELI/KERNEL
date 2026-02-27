#include "profiling.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Kernel Profiling */
static bool profiling_enabled = false;
static spinlock_t profiling_lock = SPINLOCK_INIT;

void profiling_init(void) {
    profiling_enabled = true;
    printk("[Kernel Profiling] Initialized\n");
}

/* TODO: Implement Kernel Profiling functionality */
