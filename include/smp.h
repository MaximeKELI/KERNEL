#ifndef SMP_H
#define SMP_H

#include "types.h"

/* Per-CPU data structure */
typedef struct percpu {
    u32 cpu_id;
    void* stack;
    void* gdt;
    void* idt;
    void* tss;
    u64 apic_id;
    bool online;
    void* private_data;
} percpu_t;

#define MAX_CPUS 64

/* Initialize SMP */
void smp_init(void);

/* Get current CPU ID */
u32 smp_get_cpu_id(void);

/* Get per-CPU data */
percpu_t* smp_get_percpu(u32 cpu_id);

/* Set CPU online */
void smp_set_online(u32 cpu_id);

/* Send IPI to CPU */
void smp_send_ipi(u32 cpu_id, u8 vector);

/* Broadcast IPI to all CPUs */
void smp_broadcast_ipi(u8 vector);

/* CPU affinity */
int set_cpu_affinity(u64 pid, u64 mask);
u64 get_cpu_affinity(u64 pid);

#endif /* SMP_H */
