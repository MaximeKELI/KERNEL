#include "hotplug.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "smp.h"

static hotplug_callback_t hotplug_callbacks[16] = {0};
static void* hotplug_data[16] = {0};
static u32 callback_count = 0;
static spinlock_t hotplug_lock = SPINLOCK_INIT;

void hotplug_init(void) {
    DEBUG_INFO("Hotplug system initialized");
}

int hotplug_register_callback(hotplug_callback_t callback, void* data) {
    if (!callback || callback_count >= 16) return -1;
    
    spinlock_lock(&hotplug_lock);
    hotplug_callbacks[callback_count] = callback;
    hotplug_data[callback_count] = data;
    callback_count++;
    spinlock_unlock(&hotplug_lock);
    
    return 0;
}

int cpu_hotplug_add(u32 cpu_id) {
    DEBUG_INFO("CPU hotplug add: CPU %u", cpu_id);
    
    /* Notify callbacks */
    spinlock_lock(&hotplug_lock);
    for (u32 i = 0; i < callback_count; i++) {
        if (hotplug_callbacks[i]) {
            hotplug_callbacks[i](HOTPLUG_CPU_ADD, hotplug_data[i]);
        }
    }
    spinlock_unlock(&hotplug_lock);
    
    /* Would initialize CPU here */
    smp_set_online(cpu_id);
    
    return 0;
}

int cpu_hotplug_remove(u32 cpu_id) {
    DEBUG_INFO("CPU hotplug remove: CPU %u", cpu_id);
    
    /* Notify callbacks */
    spinlock_lock(&hotplug_lock);
    for (u32 i = 0; i < callback_count; i++) {
        if (hotplug_callbacks[i]) {
            hotplug_callbacks[i](HOTPLUG_CPU_REMOVE, hotplug_data[i]);
        }
    }
    spinlock_unlock(&hotplug_lock);
    
    /* Would offline CPU here */
    return 0;
}

int memory_hotplug_add(u64 start, u64 size) {
    DEBUG_INFO("Memory hotplug add: 0x%p, size %u MB", 
               (void*)start, (u32)(size / (1024 * 1024)));
    
    /* Notify callbacks */
    spinlock_lock(&hotplug_lock);
    for (u32 i = 0; i < callback_count; i++) {
        if (hotplug_callbacks[i]) {
            hotplug_callbacks[i](HOTPLUG_MEM_ADD, hotplug_data[i]);
        }
    }
    spinlock_unlock(&hotplug_lock);
    
    /* Would add memory to system here */
    return 0;
}

int memory_hotplug_remove(u64 start, u64 size) {
    DEBUG_INFO("Memory hotplug remove: 0x%p, size %u MB",
               (void*)start, (u32)(size / (1024 * 1024)));
    
    /* Notify callbacks */
    spinlock_lock(&hotplug_lock);
    for (u32 i = 0; i < callback_count; i++) {
        if (hotplug_callbacks[i]) {
            hotplug_callbacks[i](HOTPLUG_MEM_REMOVE, hotplug_data[i]);
        }
    }
    spinlock_unlock(&hotplug_lock);
    
    /* Would remove memory from system here */
    return 0;
}
