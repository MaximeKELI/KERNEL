#ifndef MEMORY_HOTPLUG_H
#define MEMORY_HOTPLUG_H

#include "types.h"

/* Memory hotplug states */
#define MEMORY_STATE_OFFLINE  0
#define MEMORY_STATE_ONLINE   1
#define MEMORY_STATE_GOING_OFFLINE 2

/* Memory block structure */
typedef struct memory_block {
    void* start;
    size_t size;
    u32 state;
    u32 node_id; /* NUMA node */
    struct memory_block* next;
} memory_block_t;

/* Add memory block */
int memory_block_add(void* start, size_t size, u32 node_id);

/* Remove memory block */
int memory_block_remove(void* start);

/* Online memory block */
int memory_block_online(void* start);

/* Offline memory block */
int memory_block_offline(void* start);

/* Get memory block state */
u32 memory_block_get_state(void* start);

/* List all memory blocks */
memory_block_t* memory_block_list(void);

#endif /* MEMORY_HOTPLUG_H */
