#ifndef BOOT_PROFILER_H
#define BOOT_PROFILER_H

#include "types.h"

void boot_profiler_reset(void);
void boot_profiler_mark(const char* phase);
u64 boot_profiler_ms(const char* phase);
u64 boot_profiler_total_ms(void);
void boot_profiler_report(void);

#endif /* BOOT_PROFILER_H */
