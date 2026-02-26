#include "spinlock.h"
#include "interrupt.h"
#include "io.h"

void spinlock_init(spinlock_t* lock) {
    lock->locked = 0;
}

void spinlock_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        /* Wait for lock to be released */
        while (lock->locked) {
            asm volatile("pause");  /* CPU hint for spin-wait loop */
        }
    }
}

void spinlock_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->locked);
}

bool spinlock_trylock(spinlock_t* lock) {
    return __sync_lock_test_and_set(&lock->locked, 1) == 0;
}

bool spinlock_is_locked(spinlock_t* lock) {
    return lock->locked != 0;
}

void spinlock_lock_irq(spinlock_t* lock) {
    disable_interrupts();
    spinlock_lock(lock);
}

void spinlock_unlock_irq(spinlock_t* lock) {
    spinlock_unlock(lock);
    enable_interrupts();
}
