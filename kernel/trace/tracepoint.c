#include "tracepoint.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Kernel Tracepoints */
static bool tracepoint_enabled = false;
static spinlock_t tracepoint_lock = SPINLOCK_INIT;

void tracepoint_init(void) {
    tracepoint_enabled = true;
    printk("[Kernel Tracepoints] Initialized\n");
}

/* TODO: Implement Kernel Tracepoints functionality */
