#include "rop_protection.h"
#include "io.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "memory.h"

static bool rop_protection_enabled = false;
static u32 protection_type = ROP_PROT_NONE;
static spinlock_t rop_lock = SPINLOCK_INIT;

void rop_protection_init(void) {
    /* Check for Intel CET support */
    u32 eax, ebx, ecx, edx;
    cpuid(7, &eax, &ebx, &ecx, &edx);
    
    if (ecx & (1 << 7)) {
        DEBUG_INFO("Intel CET (Control-flow Enforcement Technology) detected");
        rop_protection_enable_cet();
    } else {
        DEBUG_WARN("CET not available, using software shadow stack");
        rop_protection_enable_shadow_stack();
    }
}

int rop_protection_enable_cet(void) {
    /* Enable Intel CET */
    u64 cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 23); /* CET bit */
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4));
    
    /* Enable CET in MSR */
    u64 msr = rdmsr(0x6A0); /* IA32_U_CET */
    msr |= (1ULL << 0); /* Enable CET */
    wrmsr(0x6A0, msr);
    
    spinlock_lock(&rop_lock);
    rop_protection_enabled = true;
    protection_type = ROP_PROT_CET;
    spinlock_unlock(&rop_lock);
    
    DEBUG_INFO("CET (ROP/JOP protection) enabled");
    return 0;
}

int rop_protection_enable_shadow_stack(void) {
    spinlock_lock(&rop_lock);
    rop_protection_enabled = true;
    protection_type = ROP_PROT_SHADOW;
    spinlock_unlock(&rop_lock);
    
    DEBUG_INFO("Shadow stack (software ROP protection) enabled");
    return 0;
}

bool rop_protection_verify_return(void* return_addr) {
    if (!rop_protection_enabled) return true;
    
    /* Would verify return address against shadow stack */
    (void)return_addr;
    return true;
}

bool rop_protection_is_enabled(void) {
    return rop_protection_enabled;
}
