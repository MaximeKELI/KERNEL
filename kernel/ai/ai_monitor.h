/**
 * @file ai_monitor.h
 * @brief AI Monitor - System Metrics Collection
 * @ingroup ai
 */

#ifndef AI_MONITOR_H
#define AI_MONITOR_H

#include "ai_manager.h"

/**
 * @brief Initialize AI monitor
 * 
 * Initializes the metrics collection system.
 */
void ai_monitor_init(void);

/**
 * @brief Update metrics (alias for ai_update_metrics)
 * 
 * @see ai_update_metrics()
 */
void ai_monitor_update(void);

/**
 * @brief Update metrics - called from timer interrupt
 * 
 * Collects and updates all system metrics:
 * - CPU usage
 * - Memory usage
 * - Process count
 * - Context switch rate
 * - Interrupt rate
 * 
 * @note This function is interrupt-safe and thread-safe.
 */
void ai_update_metrics(void);

/**
 * @brief Get current metrics
 * 
 * Retrieves a copy of the current metrics.
 * 
 * @param metrics Pointer to metrics structure to fill
 * 
 * @note This function is thread-safe.
 */
void ai_monitor_get_metrics(ai_metrics_t* metrics);

#endif /* AI_MONITOR_H */
