#ifndef FTRACE_H
#define FTRACE_H

#include "types.h"

/* Function tracing */
typedef struct ftrace_entry {
    u64 timestamp;
    u64 ip;
    u64 parent_ip;
    char func_name[64];
} ftrace_entry_t;

/* Initialize ftrace */
void ftrace_init(void);

/* Enable tracing */
int ftrace_enable(void);

/* Disable tracing */
int ftrace_disable(void);

/* Trace function entry */
void ftrace_function_entry(void* func);

/* Trace function exit */
void ftrace_function_exit(void* func);

/* Read trace buffer */
int ftrace_read(ftrace_entry_t* entries, u32 count);

/* Clear trace */
void ftrace_clear(void);

#endif /* FTRACE_H */
