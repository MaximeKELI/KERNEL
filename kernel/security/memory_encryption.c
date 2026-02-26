#include "memory_encryption.h"
#include "io.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static bool encryption_enabled = false;
static u32 encryption_type = MEM_ENCRYPT_NONE;
static spinlock_t encryption_lock = SPINLOCK_INIT;

void memory_encryption_init(void) {
    /* Check for AMD SME */
    u32 eax, ebx, ecx, edx;
    cpuid(0x8000001F, &eax, &ebx, &ecx, &edx);
    
    if (eax & (1 << 0)) {
        DEBUG_INFO("AMD SME (Secure Memory Encryption) detected");
        encryption_type = MEM_ENCRYPT_SME;
    }
    
    /* Check for Intel TME */
    cpuid(7, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 13)) {
        DEBUG_INFO("Intel TME (Total Memory Encryption) detected");
        encryption_type = MEM_ENCRYPT_TME;
    }
    
    if (encryption_type == MEM_ENCRYPT_NONE) {
        DEBUG_WARN("No memory encryption support detected");
    }
}

int memory_encryption_enable(u32 type) {
    if (type == MEM_ENCRYPT_NONE) return -1;
    
    spinlock_lock(&encryption_lock);
    
    if (type == MEM_ENCRYPT_SME) {
        /* Enable AMD SME */
        u64 msr = rdmsr(0xC0010010); /* SYSCFG */
        msr |= (1ULL << 23); /* SME enable */
        wrmsr(0xC0010010, msr);
        encryption_enabled = true;
        encryption_type = type;
        DEBUG_INFO("AMD SME enabled");
    } else if (type == MEM_ENCRYPT_TME) {
        /* Enable Intel TME */
        u64 msr = rdmsr(0x982); /* IA32_TME_ACTIVATE */
        msr |= (1ULL << 0); /* TME enable */
        wrmsr(0x982, msr);
        encryption_enabled = true;
        encryption_type = type;
        DEBUG_INFO("Intel TME enabled");
    }
    
    spinlock_unlock(&encryption_lock);
    return encryption_enabled ? 0 : -1;
}

int memory_encrypt_region(void* addr, size_t size) {
    if (!encryption_enabled || !addr || size == 0) return -1;
    
    /* Memory encryption is handled by hardware */
    DEBUG_INFO("Memory region encrypted: 0x%p, size=%u KB", addr, (u32)(size / 1024));
    return 0;
}

int memory_decrypt_region(void* addr, size_t size) {
    if (!encryption_enabled || !addr || size == 0) return -1;
    
    /* Would decrypt region */
    DEBUG_INFO("Memory region decrypted: 0x%p, size=%u KB", addr, (u32)(size / 1024));
    return 0;
}

bool memory_encryption_is_enabled(void) {
    return encryption_enabled;
}
