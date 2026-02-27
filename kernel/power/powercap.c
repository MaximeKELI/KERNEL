#include "powercap.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Power Capping */
static bool powercap_enabled = false;
static spinlock_t powercap_lock = SPINLOCK_INIT;

void powercap_init(void) {
    powercap_enabled = true;
    printk("[Power Capping] Initialized\n");
}

/* TODO: Implement Power Capping functionality */
