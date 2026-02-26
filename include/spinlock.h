#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

/* Spinlock structure */
typedef struct {
    volatile u32 locked;
} spinlock_t;

/* Initialize spinlock */
#define SPINLOCK_INIT {0}

/* Lock operations */
void spinlock_init(spinlock_t* lock);
void spinlock_lock(spinlock_t* lock);
void spinlock_unlock(spinlock_t* lock);
bool spinlock_trylock(spinlock_t* lock);
bool spinlock_is_locked(spinlock_t* lock);

/* IRQ-safe lock (disables interrupts) */
void spinlock_lock_irq(spinlock_t* lock);
void spinlock_unlock_irq(spinlock_t* lock);

#endif /* SPINLOCK_H */
