#ifndef MEMORY_BANDWIDTH_CTRL_H
#define MEMORY_BANDWIDTH_CTRL_H

#include "types.h"

void memory_bandwidth_ctrl_init(void);
void memory_bandwidth_account_read(u64 bytes);
void memory_bandwidth_account_write(u64 bytes);
bool memory_bandwidth_allow(u64 bytes, bool is_write);
void memory_bandwidth_get_stats(u64* read_bytes, u64* write_bytes);

#endif /* MEMORY_BANDWIDTH_CTRL_H */
