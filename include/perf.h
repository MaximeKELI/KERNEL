#ifndef PERF_H
#define PERF_H

#include "types.h"

/* Performance event types */
#define PERF_TYPE_HARDWARE 0
#define PERF_TYPE_SOFTWARE 1
#define PERF_TYPE_HW_CACHE 2

/* Hardware events */
#define PERF_COUNT_HW_CPU_CYCLES 0
#define PERF_COUNT_HW_INSTRUCTIONS 1
#define PERF_COUNT_HW_CACHE_REFERENCES 2
#define PERF_COUNT_HW_CACHE_MISSES 3

/* Performance event */
typedef struct perf_event {
    u32 type;
    u64 config;
    u64 count;
    u64 enabled;
    u64 running;
    void* private_data;
    struct perf_event* next;
} perf_event_t;

/* Initialize perf */
void perf_init(void);

/* Open perf event */
perf_event_t* perf_event_open(u32 type, u64 config, u64 pid, u32 cpu);

/* Enable perf event */
int perf_event_enable(perf_event_t* event);

/* Disable perf event */
int perf_event_disable(perf_event_t* event);

/* Read perf event */
u64 perf_event_read(perf_event_t* event);

#endif /* PERF_H */
