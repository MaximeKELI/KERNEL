#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "refcount.h"
#include "spinlock.h"

/* Process states */
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE,
    PROCESS_DEAD
} process_state_t;

struct process;

/*
 * CFS runqueue node, embedded in every process_t so the scheduler never has to
 * allocate on the hot path (cf. Linux sched_entity.run_node). Keyed by vruntime.
 */
typedef struct sched_node {
    u64 vruntime;
    struct sched_node* left;
    struct sched_node* right;
    struct sched_node* parent;
    u8 color;   /* 0 = black, 1 = red */
    u8 on_rq;   /* currently linked in the runqueue tree */
    struct process* proc;
} sched_node_t;

/*
 * A wait queue is a singly linked list of tasks blocked waiting for an event,
 * threaded through process_t.wait_next (Linux-style, minus the callbacks).
 */
typedef struct wait_queue {
    struct process* head;
} wait_queue_t;

#define WAIT_QUEUE_INIT { NULL }

/* Process structure */
typedef struct process {
    refcount_t refcount;  /* Reference counting */
    u64 pid;
    u64 parent_pid;
    process_state_t state;
    
    /*
     * CPU context.
     * `rsp` is the SAVED KERNEL STACK POINTER: switch_to() stores the outgoing
     * task's rsp here and reloads the incoming task's rsp from it. It is the
     * single invariant the context switch relies on.
     */
    u64 rsp;
    u64 rbp;
    u64 rip;
    u64 rflags;
    u64 cr3;  /* Page directory */
    
    /* Memory */
    void* stack_base;
    size_t stack_size;
    struct mm_struct* mm;   /* user virtual-memory areas (NULL for pure kthreads) */
    
    /* Scheduling */
    sched_node_t sched_node;  /* embedded CFS runqueue node (no alloc on schedule) */
    u32 time_slice_left;      /* remaining PIT ticks before preemption */
    u64 priority;
    u64 time_slice;
    u64 runtime;

    /* AI adaptive scheduler (kernel/ai) */
    u8 ai_class;
    u8 ai_boost;
    u8 ai_score;
    u16 ai_wait_ticks;
    u16 ai_run_ticks;
    
    /* User/Group IDs */
    u32 uid;
    u32 gid;
    
    /* File descriptors */
    void* files;

    /* Per-process lock (PI mutex, etc.) */
    spinlock_t lock;

    /* Opaque per-process data (signals, namespaces, ...) */
    void* private_data;

    /* Child returns 0 from SYS_FORK once */
    u8 fork_child_ret;

    /* Task lifecycle: blocking / sleep / exit */
    struct process* wait_next;  /* link when blocked on a wait queue or sleeper list */
    wait_queue_t exit_wq;       /* tasks blocked in thread_join() on this task */
    u64 sleep_until;            /* wakeup tick deadline when sleeping (sched_sleep) */
    int exit_status;            /* value passed to kthread_exit(), read by thread_join() */

    /* Next in list */
    struct process* next;
} process_t;

/* Thread structure */
typedef struct thread {
    u64 tid;
    process_t* process;
    u64 rsp;
    u64 rbp;
    u64 rip;
    u64 rflags;
    struct thread* next;
} thread_t;

/* Initialize process manager */
void process_init(void);

/* Create process */
process_t* process_create(void* entry_point, u64 stack_size);
void process_destroy(process_t* proc);

/* Get current process */
process_t* process_current(void);

/* Schedule next process */
void schedule(void);

/* Yield CPU */
void yield(void);

/* Idle task */
void idle_task(void);

/* Advanced process operations */
process_t* fork_process(void);
int exec_process(const char* path, char* const argv[]);
int wait_process(u64 pid, int* status);

#endif /* PROCESS_H */
