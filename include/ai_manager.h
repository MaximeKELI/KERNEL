/**
 * @file ai_manager.h
 * @brief AI Manager - Adaptive Intelligence Subsystem
 * @defgroup ai AI Subsystem
 * @{
 */

#ifndef AI_MANAGER_H
#define AI_MANAGER_H

#include "types.h"

/**
 * @brief AI metrics structure
 * 
 * Contains system metrics collected by the AI monitor:
 * - CPU usage percentage (0-100)
 * - Memory usage percentage (0-100)
 * - Active process count
 * - Context switch rate (per second)
 * - Interrupt rate (per second)
 */
typedef struct {
    u64 cpu_usage;           /**< CPU usage percentage (0-100) */
    u64 memory_usage;        /**< Memory usage percentage (0-100) */
    u64 process_count;       /**< Number of active processes */
    u64 context_switches;    /**< Context switch rate (per second) */
    u64 interrupt_rate;      /**< Interrupt rate (per second) */
} ai_metrics_t;

/**
 * @brief Initialize AI subsystem
 * 
 * Initializes the AI monitor and optimizer subsystems.
 * Must be called after scheduler and memory manager initialization.
 * 
 * @note This function is thread-safe and idempotent.
 */
void ai_init(void);

/**
 * @brief AI tick - called from timer interrupt
 * 
 * Updates metrics and runs optimizations. This function is called
 * periodically from the timer interrupt handler.
 * 
 * @note This function is interrupt-safe and non-blocking.
 */
void ai_tick(void);

/**
 * @brief Get current AI metrics
 * 
 * Retrieves the current system metrics collected by the AI monitor.
 * 
 * @param metrics Pointer to metrics structure to fill
 * 
 * @note This function is thread-safe.
 */
void ai_get_metrics(ai_metrics_t* metrics);

/**
 * @brief Check if AI subsystem is initialized
 * 
 * @return true if initialized, false otherwise
 */
extern bool ai_initialized;

/**
 * @}
 */

#endif /* AI_MANAGER_H */
