#include "netfilter.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Netfilter Framework */
static bool netfilter_enabled = false;
static spinlock_t netfilter_lock = SPINLOCK_INIT;

void netfilter_init(void) {
    netfilter_enabled = true;
    printk("[Netfilter Framework] Initialized\n");
}

/* TODO: Implement Netfilter Framework functionality */
