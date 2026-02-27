#include "usb_core.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* USB Core */
static bool usb_core_enabled = false;
static spinlock_t usb_core_lock = SPINLOCK_INIT;

void usb_init(void) {
    usb_core_enabled = true;
    printk("[USB Core] Initialized\n");
}

/* TODO: Implement USB Core functionality */
