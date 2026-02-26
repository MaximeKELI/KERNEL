#include "smp.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "process.h"

static percpu_t percpu_data[MAX_CPUS];
static u32 cpu_count = 1;
static spinlock_t smp_lock = SPINLOCK_INIT;

void smp_init(void) {
    /* Initialize BSP (Bootstrap Processor) */
    percpu_data[0].cpu_id = 0;
    percpu_data[0].online = true;
    percpu_data[0].apic_id = 0;
    
    /* Would detect and initialize APs (Application Processors) here */
    cpu_count = 1;
    
    DEBUG_INFO("SMP initialized: %u CPU(s)", cpu_count);
}

u32 smp_get_cpu_id(void) {
    /* Would read from MSR or APIC */
    return 0;
}

percpu_t* smp_get_percpu(u32 cpu_id) {
    if (cpu_id >= MAX_CPUS) return NULL;
    return &percpu_data[cpu_id];
}

void smp_set_online(u32 cpu_id) {
    if (cpu_id >= MAX_CPUS) return;
    
    spinlock_lock(&smp_lock);
    percpu_data[cpu_id].online = true;
    spinlock_unlock(&smp_lock);
    
    DEBUG_INFO("CPU %u set online", cpu_id);
}

void smp_send_ipi(u32 cpu_id, u8 vector) {
    (void)cpu_id;
    (void)vector;
    /* Would send IPI via APIC */
    DEBUG_INFO("Sending IPI %u to CPU %u", vector, cpu_id);
}

void smp_broadcast_ipi(u8 vector) {
    for (u32 i = 0; i < cpu_count; i++) {
        if (percpu_data[i].online) {
            smp_send_ipi(i, vector);
        }
    }
}

int set_cpu_affinity(u64 pid, u64 mask) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            proc->private_data = (void*)mask;
            return 0;
        }
        proc = proc->next;
    }
    return -1;
}

u64 get_cpu_affinity(u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            return (u64)proc->private_data;
        }
        proc = proc->next;
    }
    return 0;
}
