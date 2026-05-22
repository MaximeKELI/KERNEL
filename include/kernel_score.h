#ifndef KERNEL_SCORE_H
#define KERNEL_SCORE_H

#include "types.h"

typedef struct {
    u32 modules_full;
    u32 modules_partial;
    u32 modules_stub;
    u32 modules_total;
    u64 boot_minimal_ms;
    u64 boot_extended_ms;
    bool network_ready;
    bool ai_active;
    bool hardware_nic;
} kernel_score_t;

void kernel_score_collect(kernel_score_t* out);
void kernel_score_print(void);

#endif /* KERNEL_SCORE_H */
