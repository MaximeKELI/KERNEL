#include "kgdb.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Kernel Debugging Interface */
static bool kgdb_enabled = false;
static spinlock_t kgdb_lock = SPINLOCK_INIT;

void kgdb_init(void) {
    kgdb_enabled = true;
    printk("[Kernel Debugging Interface] Initialized\n");
}

/* TODO: Implement Kernel Debugging Interface functionality */
