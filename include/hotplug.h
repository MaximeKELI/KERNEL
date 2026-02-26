#ifndef HOTPLUG_H
#define HOTPLUG_H

#include "types.h"

/* Hotplug events */
#define HOTPLUG_CPU_ADD    1
#define HOTPLUG_CPU_REMOVE 2
#define HOTPLUG_MEM_ADD    3
#define HOTPLUG_MEM_REMOVE 4

/* Hotplug callback */
typedef void (*hotplug_callback_t)(u32 event, void* data);

/* Initialize hotplug */
void hotplug_init(void);

/* Register hotplug callback */
int hotplug_register_callback(hotplug_callback_t callback, void* data);

/* CPU hotplug */
int cpu_hotplug_add(u32 cpu_id);
int cpu_hotplug_remove(u32 cpu_id);

/* Memory hotplug */
int memory_hotplug_add(u64 start, u64 size);
int memory_hotplug_remove(u64 start, u64 size);

#endif /* HOTPLUG_H */
