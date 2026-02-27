#include "ethernet.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Ethernet Drivers */
static bool ethernet_enabled = false;
static spinlock_t ethernet_lock = SPINLOCK_INIT;

void ethernet_init(void) {
    ethernet_enabled = true;
    printk("[Ethernet Drivers] Initialized\n");
}

/* TODO: Implement Ethernet Drivers functionality */
