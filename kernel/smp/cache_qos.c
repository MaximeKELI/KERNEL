#include "cache_qos.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "process.h"

#define MAX_QOS_ENTRIES 1024

typedef struct cache_qos_entry {
    u64 pid;
    u32 class_id;
    u64 max_occupancy;
    u64 bandwidth_limit;
    u64 current_occupancy;
    bool active;
    struct cache_qos_entry* next;
} cache_qos_entry_t;

static cache_qos_entry_t* qos_table = NULL;
static spinlock_t qos_lock = SPINLOCK_INIT;

int cache_qos_init(void) {
    DEBUG_INFO("%s", "Cache QoS initialized");
    return 0;
}

int cache_qos_set_allocation(u64 pid, u32 class_id, u64 max_occupancy) {
    if (class_id > CACHE_QOS_CLASS_MB) {
        return -1;
    }
    
    spinlock_lock(&qos_lock);
    
    /* Find or create entry */
    cache_qos_entry_t* entry = qos_table;
    while (entry) {
        if (entry->pid == pid) {
            entry->class_id = class_id;
            entry->max_occupancy = max_occupancy;
            spinlock_unlock(&qos_lock);
            DEBUG_INFO("Cache QoS allocation set: pid=%u, class=%u, max=%llu",
                      pid, class_id, (unsigned long long)max_occupancy);
            return 0;
        }
        entry = entry->next;
    }
    
    /* Create new entry */
    entry = (cache_qos_entry_t*)kmalloc(sizeof(cache_qos_entry_t));
    if (!entry) {
        spinlock_unlock(&qos_lock);
        return -1;
    }
    
    entry->pid = pid;
    entry->class_id = class_id;
    entry->max_occupancy = max_occupancy;
    entry->bandwidth_limit = 0;
    entry->current_occupancy = 0;
    entry->active = true;
    entry->next = qos_table;
    qos_table = entry;
    
    spinlock_unlock(&qos_lock);
    DEBUG_INFO("Cache QoS entry created: pid=%u", pid);
    return 0;
}

int cache_qos_get_allocation(u64 pid, u32* class_id, u64* max_occupancy) {
    VALIDATE_PTR_RET(class_id, -1);
    VALIDATE_PTR_RET(max_occupancy, -1);
    
    spinlock_lock(&qos_lock);
    
    cache_qos_entry_t* entry = qos_table;
    while (entry) {
        if (entry->pid == pid && entry->active) {
            *class_id = entry->class_id;
            *max_occupancy = entry->max_occupancy;
            spinlock_unlock(&qos_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&qos_lock);
    return -1;
}

int cache_qos_monitor(u64 pid, cache_qos_monitor_t* monitor) {
    VALIDATE_PTR_RET(monitor, -1);
    
    spinlock_lock(&qos_lock);
    
    cache_qos_entry_t* entry = qos_table;
    while (entry) {
        if (entry->pid == pid && entry->active) {
            monitor->class_id = entry->class_id;
            monitor->llc_occupancy = entry->current_occupancy;
            monitor->memory_bandwidth = entry->bandwidth_limit;
            
            if (entry->max_occupancy > 0) {
                monitor->utilization_percent = 
                    (u32)((entry->current_occupancy * 100) / entry->max_occupancy);
            } else {
                monitor->utilization_percent = 0;
            }
            
            spinlock_unlock(&qos_lock);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&qos_lock);
    return -1;
}

int cache_qos_set_bandwidth_limit(u64 pid, u64 limit_mbps) {
    spinlock_lock(&qos_lock);
    
    cache_qos_entry_t* entry = qos_table;
    while (entry) {
        if (entry->pid == pid && entry->active) {
            entry->bandwidth_limit = limit_mbps;
            spinlock_unlock(&qos_lock);
            DEBUG_INFO("Cache QoS bandwidth limit set: pid=%u, limit=%llu MB/s",
                      pid, (unsigned long long)limit_mbps);
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&qos_lock);
    return -1;
}
