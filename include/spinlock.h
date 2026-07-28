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

/* Save/restore variant. Required for any lock also taken by an interrupt
 * handler: unlock_irq() re-enables interrupts unconditionally, which is wrong
 * inside a handler and in nested critical sections. */
u64 spinlock_lock_irqsave(spinlock_t* lock);
void spinlock_unlock_irqrestore(spinlock_t* lock, u64 flags);

#endif /* SPINLOCK_H */
