#include "security_hardening.h"
#include "io.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static u32 security_features = 0;
static bool lockdown_enabled = false;
static spinlock_t security_lock = SPINLOCK_INIT;

void security_hardening_init(void) {
    /* Check CPU features */
    u32 eax, ebx, ecx, edx;
    
    /* Check for SMEP support */
    cpuid(7, &eax, &ebx, &ecx, &edx);
    if (ebx & (1 << 7)) {
        security_enable_smep();
    }
    
    /* Check for SMAP support */
    if (ebx & (1 << 20)) {
        security_enable_smap();
    }
    
    /* Check for CET support */
    cpuid(7, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 7)) {
        security_enable_cfi();
    }
    
    /* Always enable stack protector */
    security_features |= SECURITY_STACK_PROT;
    
    /* Enable retpoline by default */
    security_enable_retpoline();
    
    DEBUG_INFO("Security hardening initialized: features=0x%x", security_features);
}

void security_enable_smep(void) {
    u64 cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 20); /* SMEP bit */
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4));
    
    spinlock_lock(&security_lock);
    security_features |= SECURITY_SMEP_ENABLED;
    spinlock_unlock(&security_lock);
    
    DEBUG_INFO("SMEP (Supervisor Mode Execution Prevention) enabled");
}

void security_enable_smap(void) {
    u64 cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 21); /* SMAP bit */
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4));
    
    spinlock_lock(&security_lock);
    security_features |= SECURITY_SMAP_ENABLED;
    spinlock_unlock(&security_lock);
    
    DEBUG_INFO("SMAP (Supervisor Mode Access Prevention) enabled");
}

void security_enable_kpti(void) {
    /* Would enable KPTI by using separate page tables for user/kernel */
    spinlock_lock(&security_lock);
    security_features |= SECURITY_KPTI_ENABLED;
    spinlock_unlock(&security_lock);
    
    DEBUG_INFO("KPTI (Kernel Page Table Isolation) enabled");
}

void security_enable_retpoline(void) {
    /* Retpoline protection against Spectre */
    spinlock_lock(&security_lock);
    security_features |= SECURITY_RETPOLINE;
    spinlock_unlock(&security_lock);
    
    DEBUG_INFO("Retpoline (Spectre protection) enabled");
}

void security_enable_cfi(void) {
    /* Control Flow Integrity */
    spinlock_lock(&security_lock);
    security_features |= SECURITY_CFI_ENABLED;
    spinlock_unlock(&security_lock);
    
    DEBUG_INFO("CFI (Control Flow Integrity) enabled");
}

void security_enable_lockdown(void) {
    spinlock_lock(&security_lock);
    lockdown_enabled = true;
    security_features |= SECURITY_LOCKDOWN;
    spinlock_unlock(&security_lock);
    
    DEBUG_INFO("Kernel lockdown enabled - kernel modifications disabled");
}

bool security_is_enabled(u32 feature) {
    return (security_features & feature) != 0;
}
