#ifndef CPU_ISOLATION_H
#define CPU_ISOLATION_H

#include "types.h"

/* CPU isolation modes */
#define CPU_ISOLATION_NONE     0
#define CPU_ISOLATION_FULL     1
#define CPU_ISOLATION_HOUSEKEEPING 2

/* CPU isolation flags */
#define CPU_ISOLATION_NO_TICK  0x01
#define CPU_ISOLATION_NO_RCU   0x02
#define CPU_ISOLATION_NO_MIGRATE 0x04

/* Initialize CPU isolation */
void cpu_isolation_init(void);

/* Isolate CPU */
int cpu_isolate(u32 cpu, u32 mode, u32 flags);

/* Unisolate CPU */
int cpu_unisolate(u32 cpu);

/* Check if CPU is isolated */
bool cpu_is_isolated(u32 cpu);

/* Get isolation mode */
u32 cpu_get_isolation_mode(u32 cpu);

/* Set process CPU affinity (isolated CPUs) */
int cpu_set_affinity(u64 pid, u64 cpu_mask);

/* Get process CPU affinity */
u64 cpu_get_affinity(u64 pid);

#endif /* CPU_ISOLATION_H */
