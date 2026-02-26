/**
 * @file ai_optimizer.h
 * @brief AI Optimizer - Adaptive System Optimizations
 * @ingroup ai
 */

#ifndef AI_OPTIMIZER_H
#define AI_OPTIMIZER_H

#include "types.h"

/**
 * @brief Initialize AI optimizer
 * 
 * Initializes the optimization subsystem.
 */
void ai_optimizer_init(void);

/**
 * @brief Optimize scheduler
 * 
 * Applies adaptive scheduler optimizations based on system metrics:
 * - Reduces timeslice if CPU usage > 80%
 * - Boosts I/O-bound processes if many context switches
 * - Increases timeslice if system idle (CPU < 20%)
 * 
 * @note This function is interrupt-safe and non-blocking.
 */
void ai_optimize_scheduler(void);

/**
 * @brief Optimize memory
 * 
 * Applies memory optimizations based on system metrics:
 * - Triggers cache sync if memory pressure > 85%
 * - Throttles cleanup to avoid excessive overhead
 * 
 * @note This function is interrupt-safe and non-blocking.
 */
void ai_optimize_memory(void);

/**
 * @brief Detect anomalies
 * 
 * Detects and responds to system anomalies:
 * - High CPU usage processes
 * - Abnormal interrupt rates
 * - Excessive context switches
 * 
 * @note This function is interrupt-safe and non-blocking.
 */
void ai_detect_anomalies(void);

#endif /* AI_OPTIMIZER_H */
