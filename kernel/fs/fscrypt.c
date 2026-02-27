#include "fscrypt.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Filesystem Encryption */
static bool fscrypt_enabled = false;
static spinlock_t fscrypt_lock = SPINLOCK_INIT;

void fscrypt_init(void) {
    fscrypt_enabled = true;
    printk("[Filesystem Encryption] Initialized\n");
}

/* TODO: Implement Filesystem Encryption functionality */
