#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

/* Process states */
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE,
    PROCESS_DEAD
} process_state_t;

/* Process structure */
typedef struct process {
    u64 pid;
    u64 parent_pid;
    process_state_t state;
    
    /* CPU context */
    u64 rsp;
    u64 rbp;
    u64 rip;
    u64 rflags;
    u64 cr3;  /* Page directory */
    
    /* Memory */
    void* stack_base;
    size_t stack_size;
    
    /* Scheduling */
    u64 priority;
    u64 time_slice;
    u64 runtime;
    
    /* User/Group IDs */
    u32 uid;
    u32 gid;
    
    /* File descriptors */
    void* files;
    
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

/* Context switch */
void context_switch(process_t* from, process_t* to);

/* Yield CPU */
void yield(void);

/* Idle task */
void idle_task(void);

/* Advanced process operations */
process_t* fork_process(void);
int exec_process(const char* path, char* const argv[]);
int wait_process(u64 pid, int* status);

#endif /* PROCESS_H */
