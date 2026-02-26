#include "kasan.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static bool kasan_enabled_flag = false;
static u8* kasan_shadow = NULL;
static spinlock_t kasan_lock = SPINLOCK_INIT;

#define KASAN_SHADOW_SCALE 8
#define KASAN_SHADOW_OFFSET 0x7fff8000ULL

void kasan_init(void) {
    /* Allocate shadow memory */
    size_t shadow_size = (512 * 1024 * 1024) / KASAN_SHADOW_SCALE; /* For 512MB */
    kasan_shadow = (u8*)vmm_alloc_pages((shadow_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (kasan_shadow) {
        memset(kasan_shadow, 0, shadow_size);
    }
    
    DEBUG_INFO("KASAN (Kernel Address Sanitizer) initialized");
}

void kasan_enable(void) {
    spinlock_lock(&kasan_lock);
    kasan_enabled_flag = true;
    spinlock_unlock(&kasan_lock);
    DEBUG_INFO("KASAN enabled");
}

void kasan_disable(void) {
    spinlock_lock(&kasan_lock);
    kasan_enabled_flag = false;
    spinlock_unlock(&kasan_lock);
    DEBUG_INFO("KASAN disabled");
}

void kasan_unpoison(void* addr, size_t size) {
    if (!kasan_enabled_flag || !kasan_shadow) return;
    
    u64 addr_val = (u64)addr;
    u64 shadow_addr = (addr_val >> 3) + KASAN_SHADOW_OFFSET;
    
    for (size_t i = 0; i < size; i++) {
        if (shadow_addr < (u64)kasan_shadow + (512 * 1024 * 1024) / KASAN_SHADOW_SCALE) {
            kasan_shadow[(shadow_addr - (u64)kasan_shadow) + i] = 0;
        }
    }
}

void kasan_poison(void* addr, size_t size) {
    if (!kasan_enabled_flag || !kasan_shadow) return;
    
    u64 addr_val = (u64)addr;
    u64 shadow_addr = (addr_val >> 3) + KASAN_SHADOW_OFFSET;
    
    for (size_t i = 0; i < size; i++) {
        if (shadow_addr < (u64)kasan_shadow + (512 * 1024 * 1024) / KASAN_SHADOW_SCALE) {
            kasan_shadow[(shadow_addr - (u64)kasan_shadow) + i] = 0xFF;
        }
    }
}

bool kasan_check(void* addr, size_t size) {
    if (!kasan_enabled_flag || !kasan_shadow) return true;
    
    u64 addr_val = (u64)addr;
    u64 shadow_addr = (addr_val >> 3) + KASAN_SHADOW_OFFSET;
    
    for (size_t i = 0; i < size; i++) {
        u64 offset = (shadow_addr - (u64)kasan_shadow) + i;
        if (offset < (512 * 1024 * 1024) / KASAN_SHADOW_SCALE) {
            if (kasan_shadow[offset] != 0) {
                return false;
            }
        }
    }
    
    return true;
}

void kasan_report(void* addr, size_t size, bool is_write) {
    DEBUG_ERROR("KASAN: %s at address 0x%p, size %u",
                is_write ? "write" : "read", addr, (u32)size);
}
