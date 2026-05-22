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
typedef struct __packed {
    u32 magic;
    u32 version;
    u32 cpu_usage;
    u32 memory_usage;
    u32 cpu_predict;
    u32 mem_predict;
    u32 health_score;
    u32 policy_mode;
    u32 goal_mode;
    u32 process_count;
    u32 decisions_total;
    u32 io_class_count;
    u32 net_class_count;
    u32 reserved[2];
} ai_user_info_t;

typedef struct {
    u64 cpu_usage;           /**< CPU usage percentage (0-100) */
    u64 memory_usage;        /**< Memory usage percentage (0-100) */
    u64 process_count;       /**< Number of active processes */
    u64 context_switches;    /**< Context switch rate (per second) */
    u64 interrupt_rate;      /**< Interrupt rate (per second) */
    u64 io_read_bytes;       /**< I/O read bytes (total) */
    u64 io_write_bytes;      /**< I/O write bytes (total) */
    u64 io_read_ops;         /**< I/O read operations (total) */
    u64 io_write_ops;        /**< I/O write operations (total) */
    u64 net_tx_bytes;        /**< Network TX bytes (total) */
    u64 net_rx_bytes;        /**< Network RX bytes (total) */
    u64 net_tx_packets;      /**< Network TX packets (total) */
    u64 net_rx_packets;      /**< Network RX packets (total) */
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

u32 ai_health_score(void);
void ai_fill_user_info(ai_user_info_t* out);

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
