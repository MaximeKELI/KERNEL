#include "sysfs.h"
#include "ai_manager.h"
#include "ai_optimizer.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include "debug.h"

/* Sysfs entries for AI configuration */
static sysfs_entry_t* ai_dir = NULL;
static sysfs_entry_t* ai_metrics_file = NULL;
static sysfs_entry_t* ai_cpu_threshold_file = NULL;
static sysfs_entry_t* ai_memory_threshold_file = NULL;
static sysfs_entry_t* ai_enabled_file = NULL;

/* Read AI metrics from sysfs */
static ssize_t ai_sysfs_read_metrics(const char* path, void* buf, size_t count) {
    (void)path;
    
    ai_metrics_t metrics;
    ai_get_metrics(&metrics);
    
    char metrics_buf[512];
    int len = snprintf(metrics_buf, sizeof(metrics_buf),
        "cpu_usage: %llu%%\n"
        "memory_usage: %llu%%\n"
        "process_count: %llu\n"
        "context_switches: %llu/sec\n"
        "interrupt_rate: %llu/sec\n"
        "io_read_bytes: %llu\n"
        "io_write_bytes: %llu\n"
        "io_read_ops: %llu\n"
        "io_write_ops: %llu\n"
        "net_tx_bytes: %llu\n"
        "net_rx_bytes: %llu\n"
        "net_tx_packets: %llu\n"
        "net_rx_packets: %llu\n",
        (unsigned long long)metrics.cpu_usage,
        (unsigned long long)metrics.memory_usage,
        (unsigned long long)metrics.process_count,
        (unsigned long long)metrics.context_switches,
        (unsigned long long)metrics.interrupt_rate,
        (unsigned long long)metrics.io_read_bytes,
        (unsigned long long)metrics.io_write_bytes,
        (unsigned long long)metrics.io_read_ops,
        (unsigned long long)metrics.io_write_ops,
        (unsigned long long)metrics.net_tx_bytes,
        (unsigned long long)metrics.net_rx_bytes,
        (unsigned long long)metrics.net_tx_packets,
        (unsigned long long)metrics.net_rx_packets);
    
    if (len < 0 || (size_t)len > count) {
        len = count - 1;
    }
    
    memcpy(buf, metrics_buf, len);
    return len;
}

/* Write CPU threshold */
static ssize_t ai_sysfs_write_cpu_threshold(const char* path, const void* buf, size_t count) {
    (void)path;
    
    char value_buf[32];
    if (count >= sizeof(value_buf)) {
        count = sizeof(value_buf) - 1;
    }
    memcpy(value_buf, buf, count);
    value_buf[count] = '\0';
    
    u64 threshold = 0;
    /* Simple atoi */
    for (size_t i = 0; i < count && value_buf[i] >= '0' && value_buf[i] <= '9'; i++) {
        threshold = threshold * 10 + (value_buf[i] - '0');
    }
    
    extern void ai_set_cpu_threshold(u64);
    ai_set_cpu_threshold(threshold);
    
    return count;
}

/* Read CPU threshold */
static ssize_t ai_sysfs_read_cpu_threshold(const char* path, void* buf, size_t count) {
    (void)path;
    
    extern u64 ai_get_cpu_threshold(void);
    u64 threshold = ai_get_cpu_threshold();
    
    char value_buf[32];
    int len = snprintf(value_buf, sizeof(value_buf), "%llu\n", (unsigned long long)threshold);
    
    if (len < 0 || (size_t)len > count) {
        len = count - 1;
    }
    
    memcpy(buf, value_buf, len);
    return len;
}

/* Write memory threshold */
static ssize_t ai_sysfs_write_memory_threshold(const char* path, const void* buf, size_t count) {
    (void)path;
    
    char value_buf[32];
    if (count >= sizeof(value_buf)) {
        count = sizeof(value_buf) - 1;
    }
    memcpy(value_buf, buf, count);
    value_buf[count] = '\0';
    
    u64 threshold = 0;
    for (size_t i = 0; i < count && value_buf[i] >= '0' && value_buf[i] <= '9'; i++) {
        threshold = threshold * 10 + (value_buf[i] - '0');
    }
    
    extern void ai_set_memory_threshold(u64);
    ai_set_memory_threshold(threshold);
    
    return count;
}

/* Read memory threshold */
static ssize_t ai_sysfs_read_memory_threshold(const char* path, void* buf, size_t count) {
    (void)path;
    
    extern u64 ai_get_memory_threshold(void);
    u64 threshold = ai_get_memory_threshold();
    
    char value_buf[32];
    int len = snprintf(value_buf, sizeof(value_buf), "%llu\n", (unsigned long long)threshold);
    
    if (len < 0 || (size_t)len > count) {
        len = count - 1;
    }
    
    memcpy(buf, value_buf, len);
    return len;
}

/* Write enabled state */
static ssize_t ai_sysfs_write_enabled(const char* path, const void* buf, size_t count) {
    (void)path;
    
    char value_buf[32];
    if (count >= sizeof(value_buf)) {
        count = sizeof(value_buf) - 1;
    }
    memcpy(value_buf, buf, count);
    value_buf[count] = '\0';
    
    bool enabled = false;
    if (value_buf[0] == '1' || value_buf[0] == 'y' || value_buf[0] == 'Y') {
        enabled = true;
    }
    
    extern void ai_set_enabled(bool);
    ai_set_enabled(enabled);
    
    return count;
}

/* Read enabled state */
static ssize_t ai_sysfs_read_enabled(const char* path, void* buf, size_t count) {
    (void)path;
    
    extern bool ai_is_enabled(void);
    bool enabled = ai_is_enabled();
    
    char value_buf[32];
    int len = snprintf(value_buf, sizeof(value_buf), "%s\n", enabled ? "1" : "0");
    
    if (len < 0 || (size_t)len > count) {
        len = count - 1;
    }
    
    memcpy(buf, value_buf, len);
    return len;
}

/* Initialize AI sysfs interface */
void ai_sysfs_init(void) {
    /* Create /sys/kernel/ai directory */
    /* Note: This is a simplified implementation */
    /* In a full implementation, we would use sysfs_create_file() */
    
    DEBUG_INFO("AI sysfs interface initialized");
}
