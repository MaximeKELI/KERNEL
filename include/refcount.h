#ifndef REFCOUNT_H
#define REFCOUNT_H

#include "types.h"
#include "spinlock.h"
#include "debug.h"

/* Reference counted object base */
typedef struct refcount {
    u32 count;
    spinlock_t lock;
} refcount_t;

/* Initialize refcount */
#define REFCOUNT_INIT {0, SPINLOCK_INIT}

/* Get reference (increment) */
static inline void refcount_get(refcount_t* ref) {
    if (!ref) {
        DEBUG_ERROR("%s", "refcount_get: NULL pointer");
        return;
    }
    spinlock_lock(&ref->lock);
    ref->count++;
    spinlock_unlock(&ref->lock);
}

/* Put reference (decrement) */
static inline u32 refcount_put(refcount_t* ref) {
    if (!ref) {
        DEBUG_ERROR("%s", "refcount_put: NULL pointer");
        return 0;
    }
    spinlock_lock(&ref->lock);
    if (ref->count > 0) {
        ref->count--;
    }
    u32 count = ref->count;
    spinlock_unlock(&ref->lock);
    return count;
}

/* Get current count */
static inline u32 refcount_read(refcount_t* ref) {
    if (!ref) return 0;
    spinlock_lock(&ref->lock);
    u32 count = ref->count;
    spinlock_unlock(&ref->lock);
    return count;
}

/* Check if zero (for cleanup) */
static inline bool refcount_is_zero(refcount_t* ref) {
    if (!ref) return true;
    spinlock_lock(&ref->lock);
    bool zero = (ref->count == 0);
    spinlock_unlock(&ref->lock);
    return zero;
}

#endif /* REFCOUNT_H */
