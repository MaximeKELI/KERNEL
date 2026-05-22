#include "memory_bandwidth_ctrl.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

#define BW_LIMIT_BYTES_PER_TICK (64 * 1024 * 1024)

static bool bandwidth_ctrl_enabled = false;
static u64 bytes_read = 0;
static u64 bytes_written = 0;
static spinlock_t bandwidth_ctrl_lock = SPINLOCK_INIT;

void memory_bandwidth_ctrl_init(void) {
    spinlock_lock(&bandwidth_ctrl_lock);
    bandwidth_ctrl_enabled = true;
    bytes_read = 0;
    bytes_written = 0;
    spinlock_unlock(&bandwidth_ctrl_lock);
    printk("[Memory Bandwidth Controller] Initialized\n");
}

void memory_bandwidth_account_read(u64 bytes) {
    if (!bandwidth_ctrl_enabled || bytes == 0) {
        return;
    }
    spinlock_lock(&bandwidth_ctrl_lock);
    bytes_read += bytes;
    spinlock_unlock(&bandwidth_ctrl_lock);
}

void memory_bandwidth_account_write(u64 bytes) {
    if (!bandwidth_ctrl_enabled || bytes == 0) {
        return;
    }
    spinlock_lock(&bandwidth_ctrl_lock);
    bytes_written += bytes;
    spinlock_unlock(&bandwidth_ctrl_lock);
}

bool memory_bandwidth_allow(u64 bytes, bool is_write) {
    u64 total;

    if (!bandwidth_ctrl_enabled) {
        return true;
    }

    spinlock_lock(&bandwidth_ctrl_lock);
    total = is_write ? bytes_written : bytes_read;
    spinlock_unlock(&bandwidth_ctrl_lock);

    return (total + bytes) <= BW_LIMIT_BYTES_PER_TICK;
}

void memory_bandwidth_ctrl_get_stats(u64* read_bytes, u64* write_bytes) {
    spinlock_lock(&bandwidth_ctrl_lock);
    if (read_bytes) {
        *read_bytes = bytes_read;
    }
    if (write_bytes) {
        *write_bytes = bytes_written;
    }
    spinlock_unlock(&bandwidth_ctrl_lock);
}
