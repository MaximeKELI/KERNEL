#include "drm.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* DRM Graphics */
static bool drm_enabled = false;
static spinlock_t drm_lock = SPINLOCK_INIT;

void drm_init(void) {
    drm_enabled = true;
    printk("[DRM Graphics] Initialized\n");
}

/* TODO: Implement DRM Graphics functionality */
