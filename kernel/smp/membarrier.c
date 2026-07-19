#include "membarrier.h"
#include "io.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "smp.h"

static u32 membarrier_supported = 0;
static spinlock_t membarrier_lock = SPINLOCK_INIT;

void membarrier_init(void) {
    /* Query supported commands */
    membarrier_supported = MEMBARRIER_CMD_GLOBAL |
                          MEMBARRIER_CMD_GLOBAL_EXPEDITED |
                          MEMBARRIER_CMD_PRIVATE_EXPEDITED;
    
    DEBUG_INFO("Membarrier initialized: supported=0x%x", membarrier_supported);
}

int membarrier(i32 cmd, i32 flags) {
    (void)flags;
    
    switch (cmd) {
        case MEMBARRIER_CMD_QUERY:
            return (int)membarrier_supported;
            
        case MEMBARRIER_CMD_GLOBAL:
            /* Global memory barrier across all CPUs */
            smp_mb();
            return 0;
            
        case MEMBARRIER_CMD_GLOBAL_EXPEDITED:
            /* Expedited global barrier */
            smp_mb();
            /* Would send IPI to all CPUs */
            return 0;
            
        case MEMBARRIER_CMD_PRIVATE_EXPEDITED:
            /* Private expedited barrier for current process */
            mb();
            return 0;
            
        case MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED:
        case MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED:
        case MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED_SYNC_CORE:
            /* Registration - would track in process */
            return 0;
            
        default:
            DEBUG_ERROR("Unsupported membarrier command: %d", cmd);
            return -1;
    }
}

void mb(void) {
    __asm__ __volatile__("mfence" ::: "memory");
}

void rmb(void) {
    __asm__ __volatile__("lfence" ::: "memory");
}

void wmb(void) {
    __asm__ __volatile__("sfence" ::: "memory");
}

void smp_mb(void) {
    mb();
    /* Would send IPI to all other CPUs */
}

void smp_rmb(void) {
    rmb();
}

void smp_wmb(void) {
    wmb();
}
