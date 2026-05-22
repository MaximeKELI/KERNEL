#include "mpk.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "vmm.h"
#include "io.h"

#define MPK_PKRU_BIT_OFFSET 0
#define MPK_PKRU_BIT_WD     1
#define MPK_PKRU_BIT_WR     2

static bool mpk_available = false;
static bool mpk_keys_used[MPK_KEYS_MAX] = {false};
static spinlock_t mpk_lock = SPINLOCK_INIT;

void mpk_init(void) {
    /* Check CPUID for MPK support */
    u32 eax, ebx, ecx, edx;
    cpuid(7, &eax, &ebx, &ecx, &edx);
    
    if (ecx & (1 << 3)) { /* PKU bit */
        mpk_available = true;
        DEBUG_INFO("%s", "Memory Protection Keys (MPK) available");
    } else {
        DEBUG_WARN("%s", "Memory Protection Keys (MPK) not available");
    }
}

int mpk_alloc_key(void) {
    if (!mpk_available) {
        return -1;
    }
    
    spinlock_lock(&mpk_lock);
    
    for (u32 i = 0; i < MPK_KEYS_MAX; i++) {
        if (!mpk_keys_used[i]) {
            mpk_keys_used[i] = true;
            spinlock_unlock(&mpk_lock);
            DEBUG_INFO("MPK key allocated: %u", i);
            return (int)i;
        }
    }
    
    spinlock_unlock(&mpk_lock);
    DEBUG_ERROR("%s", "No free MPK keys available");
    return -1;
}

int mpk_free_key(u32 key) {
    if (key >= MPK_KEYS_MAX) {
        return -1;
    }
    
    spinlock_lock(&mpk_lock);
    
    if (!mpk_keys_used[key]) {
        spinlock_unlock(&mpk_lock);
        return -1;
    }
    
    mpk_keys_used[key] = false;
    spinlock_unlock(&mpk_lock);
    
    DEBUG_INFO("MPK key freed: %u", key);
    return 0;
}

int mpk_set_prot(void* addr, size_t len, u32 key, u32 prot) {
    VALIDATE_PTR_RET(addr, -1);
    
    if (!mpk_available || key >= MPK_KEYS_MAX) {
        return -1;
    }
    
    /* Set PKRU register */
    u32 pkru = rdmsr(0x1E4); /* PKRU MSR */
    
    u32 bit_offset = key * 2;
    if (prot & MPK_PROT_WRITE) {
        pkru &= ~(1 << (bit_offset + MPK_PKRU_BIT_WD));
    } else {
        pkru |= (1 << (bit_offset + MPK_PKRU_BIT_WD));
    }
    
    wrmsr(0x1E4, pkru);
    
    DEBUG_INFO("MPK protection set: addr=0x%p, key=%u, prot=0x%x", addr, key, prot);
    return 0;
    
    (void)len;
}

u32 mpk_get_prot(void* addr, u32* key) {
    VALIDATE_PTR_RET(addr, 0);
    
    if (!mpk_available) {
        return 0;
    }
    
    /* Would read from page table entry */
    (void)(u32)rdmsr(0x1E4); /* PKRU - protection keys */
    
    if (key) {
        *key = 0; /* Would extract from PTE */
    }
    
    return MPK_PROT_READ | MPK_PROT_WRITE | MPK_PROT_EXEC;
}
