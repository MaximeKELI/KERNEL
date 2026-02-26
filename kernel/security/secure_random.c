#include "secure_random.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "io.h"
#include "drivers/timer.h"

#define RANDOM_POOL_SIZE 4096
static u8 random_pool[RANDOM_POOL_SIZE];
static u32 pool_index = 0;
static u64 entropy_counter = 0;
static spinlock_t random_lock = SPINLOCK_INIT;

/* Simple PRNG with entropy mixing */
static u64 prng_state = 0x123456789ABCDEF0ULL;

static u64 prng_next(void) {
    prng_state = prng_state * 1103515245ULL + 12345ULL;
    prng_state ^= timer_get_ticks();
    prng_state = (prng_state << 13) | (prng_state >> 51);
    return prng_state;
}

void secure_random_init(void) {
    /* Initialize with timer-based entropy */
    u64 seed = timer_get_ticks();
    seed ^= (u64)timer_get_ticks() << 32;
    
    /* Mix with CPUID */
    u32 eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    seed ^= ((u64)edx << 32) | eax;
    
    prng_state = seed;
    
    /* Fill pool */
    for (u32 i = 0; i < RANDOM_POOL_SIZE; i++) {
        random_pool[i] = (u8)prng_next();
    }
    
    DEBUG_INFO("Secure random generator initialized");
}

int secure_random_get_bytes(u8* buffer, size_t size) {
    if (!buffer || size == 0) return -1;
    
    spinlock_lock(&random_lock);
    
    for (size_t i = 0; i < size; i++) {
        /* Mix pool with PRNG */
        random_pool[pool_index] ^= (u8)prng_next();
        buffer[i] = random_pool[pool_index];
        pool_index = (pool_index + 1) % RANDOM_POOL_SIZE;
        
        /* Add entropy from timer */
        if (i % 8 == 0) {
            u64 ticks = timer_get_ticks();
            random_pool[pool_index] ^= (u8)(ticks ^ (ticks >> 8) ^ (ticks >> 16));
        }
    }
    
    entropy_counter += size;
    spinlock_unlock(&random_lock);
    
    return 0;
}

u32 secure_random_u32(void) {
    u32 value;
    secure_random_get_bytes((u8*)&value, sizeof(value));
    return value;
}

u64 secure_random_u64(void) {
    u64 value;
    secure_random_get_bytes((u8*)&value, sizeof(value));
    return value;
}

void secure_random_add_entropy(const u8* data, size_t size) {
    if (!data || size == 0) return;
    
    spinlock_lock(&random_lock);
    
    for (size_t i = 0; i < size && i < RANDOM_POOL_SIZE; i++) {
        random_pool[pool_index] ^= data[i];
        pool_index = (pool_index + 1) % RANDOM_POOL_SIZE;
    }
    
    entropy_counter += size;
    spinlock_unlock(&random_lock);
}
