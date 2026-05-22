#include "net_stats.h"
#include "net.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"
#include "string.h"

static net_stats_t global_stats = {0};
static spinlock_t stats_lock = SPINLOCK_INIT;

void net_stats_init(void) {
    memset(&global_stats, 0, sizeof(global_stats));
    printk("[Net Stats] Network statistics initialized\n");
}

void net_stats_update_rx(u64 bytes) {
    spinlock_lock(&stats_lock);
    global_stats.rx_packets++;
    global_stats.rx_bytes += bytes;
    spinlock_unlock(&stats_lock);
}

void net_stats_update_tx(u64 bytes) {
    spinlock_lock(&stats_lock);
    global_stats.tx_packets++;
    global_stats.tx_bytes += bytes;
    spinlock_unlock(&stats_lock);
}

void net_stats_update_rx_error(void) {
    spinlock_lock(&stats_lock);
    global_stats.rx_errors++;
    spinlock_unlock(&stats_lock);
}

void net_stats_update_tx_error(void) {
    spinlock_lock(&stats_lock);
    global_stats.tx_errors++;
    spinlock_unlock(&stats_lock);
}

void net_stats_get(net_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    spinlock_lock(&stats_lock);
    *stats = global_stats;
    spinlock_unlock(&stats_lock);
}

void net_stats_reset(void) {
    spinlock_lock(&stats_lock);
    memset(&global_stats, 0, sizeof(global_stats));
    spinlock_unlock(&stats_lock);
}
