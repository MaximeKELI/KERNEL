#ifndef MEMORY_PRESSURE_H
#define MEMORY_PRESSURE_H

#include "types.h"

/* Memory pressure levels */
#define MEMORY_PRESSURE_LOW      0
#define MEMORY_PRESSURE_MEDIUM   1
#define MEMORY_PRESSURE_HIGH     2
#define MEMORY_PRESSURE_CRITICAL 3

/* Initialize memory pressure detection */
void memory_pressure_init(void);

/* Check current memory pressure level */
u32 memory_pressure_check(void);

/* Get current pressure level */
u32 memory_pressure_get_level(void);

/* Register callback for pressure changes */
typedef void (*pressure_callback_t)(u32 level);
bool memory_pressure_register_callback(pressure_callback_t callback);

/* Get pressure statistics */
void memory_pressure_get_stats(u64 stats[4]);

/* Get pressure level name */
const char* memory_pressure_level_name(u32 level);

#endif /* MEMORY_PRESSURE_H */
