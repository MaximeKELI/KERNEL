#ifndef EBPF_H
#define EBPF_H

#include "types.h"

/* Extended BPF */

/* Initialize Extended BPF */
void ebpf_init(void);
u64 ebpf_execute_prog(void* prog_data, size_t size, u64 arg0);

#endif /* EBPF_H */
