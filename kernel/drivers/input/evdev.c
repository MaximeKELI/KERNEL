#include "evdev.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Input Devices */
static bool evdev_enabled = false;
static spinlock_t evdev_lock = SPINLOCK_INIT;

void evdev_init(void) {
    evdev_enabled = true;
    printk("[Input Devices] Initialized\n");
}

/* TODO: Implement Input Devices functionality */
