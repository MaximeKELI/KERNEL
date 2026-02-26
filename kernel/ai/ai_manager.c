#include "ai_manager.h"
#include "ai_monitor.h"
#include "ai_optimizer.h"
#include "stdio.h"
#include "debug.h"
#include "interrupt.h"
#include "spinlock.h"
#include "validate.h"

bool ai_initialized = false;
static spinlock_t ai_init_lock = SPINLOCK_INIT;

void ai_init(void) {
    spinlock_lock(&ai_init_lock);
    
    if (ai_initialized) {
        spinlock_unlock(&ai_init_lock);
        return;
    }
    
    /* Initialize monitor */
    ai_monitor_init();
    
    /* Initialize optimizer */
    ai_optimizer_init();
    
    /* Reset metrics */
    ai_metrics_t metrics = {0};
    ai_monitor_get_metrics(&metrics);
    
    ai_initialized = true;
    spinlock_unlock(&ai_init_lock);
    
    printk("[AI] Adaptive Intelligence Initialized\n");
}

void ai_tick(void) {
    /* Fast path check - no lock needed for read */
    if (!ai_initialized) return;
    
    /* Update metrics first */
    ai_update_metrics();
    
    /* Run optimizations (non-blocking, interrupt-safe) */
    ai_optimize_scheduler();
    ai_optimize_memory();
    ai_detect_anomalies();
}

void ai_get_metrics(ai_metrics_t* metrics) {
    VALIDATE_PTR_VOID(metrics);
    ai_monitor_get_metrics(metrics);
}
