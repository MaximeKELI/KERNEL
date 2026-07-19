#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "process.h"

/* Save the current RFLAGS and disable interrupts (returns previous flags). */
static inline u64 local_irq_save(void) {
    u64 flags;
    __asm__ __volatile__("pushfq; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

/* Restore RFLAGS previously captured by local_irq_save(). */
static inline void local_irq_restore(u64 flags) {
    __asm__ __volatile__("push %0; popfq" : : "r"(flags) : "memory", "cc");
}

/* Scheduler types */
#define SCHED_NORMAL 0
#define SCHED_FIFO   1
#define SCHED_RR     2
#define SCHED_CFS    3

/* Process priority (nice value) */
#define PRIO_MIN -20
#define PRIO_MAX  19
#define PRIO_DEFAULT 0

/* Scheduler statistics */
typedef struct {
    u64 total_switches;
    u64 total_runtime;
    u64 idle_time;
} scheduler_stats_t;

/* Initialize advanced scheduler */
void scheduler_init(void);

/* Schedule next process (CFS-like) */
void schedule(void);

/* Account one timer tick of CPU time (called from the timer IRQ) */
void scheduler_tick(void);

/*
 * Preemption entry point, called from the timer IRQ after scheduler_tick().
 * Decrements the current task's quantum and reschedules when it expires and
 * preemption is enabled.
 */
void scheduler_timer_preempt(void);

/* Preemption control (Linux preempt_count semantics: >0 means non-preemptible) */
void preempt_disable(void);
void preempt_enable(void);

/*
 * Low-level kernel-stack switch (kernel/asm/context_switch.S).
 * Saves callee-saved registers of the current task, stores rsp at *save_rsp,
 * loads rsp from *load_rsp, restores callee-saved registers and returns into
 * the incoming task. Both arguments point at process_t.rsp.
 */
void switch_to(u64* save_rsp, u64* load_rsp);

/* First code every freshly-scheduled task runs (finish_task_switch): unlocks + sti */
void schedule_tail(void);

/* Make a READY task runnable and insert it into the runqueue */
void wake_up_process(process_t* proc);

/* Create a kernel thread but leave it stopped (not on the runqueue) */
process_t* kthread_create_stopped(void (*entry)(void*), void* arg, u64 stack_size);

/* Create + start a kernel thread (entry(arg)); returns the process or NULL */
process_t* kthread_run(void (*entry)(void*), void* arg, u64 stack_size);

/* Terminate the current kernel thread (never returns) */
void thread_exit(void);

/* Terminate the current kernel thread with an explicit exit code (never returns) */
void kthread_exit(int code);

/*
 * Block until `child` terminates, then reap it (free its stack and descriptor).
 * Stores the child's exit code in *status when non-NULL. Returns 0 on success.
 */
int thread_join(process_t* child, int* status);

/* Free a terminated (zombie) task's resources. Not for running/queued tasks. */
void process_reap(process_t* proc);

/* Wake a parent blocked in wait_process() when one of its children exits. */
void process_notify_parent_exit(process_t* child);

/* Set process priority */
int setpriority(u64 pid, int priority);

/* Get process priority */
int getpriority(u64 pid);

/* Get scheduler statistics */
void scheduler_get_stats(scheduler_stats_t* stats);

/* Calculate virtual runtime (for CFS) */
u64 calc_vruntime(process_t* proc);

/* Get running task count for CPU */
u64 scheduler_get_running_count(u32 cpu_id);

#endif /* SCHEDULER_H */
