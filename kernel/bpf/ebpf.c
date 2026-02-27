#include "ebpf.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

/* Extended BPF */
static bool ebpf_enabled = false;
static spinlock_t ebpf_lock = SPINLOCK_INIT;

void ebpf_init(void) {
    ebpf_enabled = true;
    printk("[Extended BPF] Initialized\n");
}

/* TODO: Implement Extended BPF functionality */
