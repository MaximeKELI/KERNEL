#include "ai_manager.h"
#include "ai_monitor.h"
#include "ai_optimizer.h"
#include "stdio.h"
#include "debug.h"
#include "interrupt.h"

bool ai_initialized = false;

void ai_init(void) {
    if (ai_initialized) return;
    
    /* Initialize monitor */
    ai_monitor_init();
    
    /* Initialize optimizer */
    ai_optimizer_init();
    
    ai_initialized = true;
    printk("[AI] Intelligent Optimization Subsystem initialized\n");
}

void ai_tick(void) {
    if (!ai_initialized) return;
    
    /* Update metrics */
    ai_monitor_update();
    
    /* Run optimizations */
    ai_optimize_scheduler();
    ai_optimize_memory();
    ai_detect_anomalies();
}

void ai_get_metrics(ai_metrics_t* metrics) {
    if (!metrics) return;
    ai_monitor_get_metrics(metrics);
}
