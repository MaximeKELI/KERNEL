#ifndef SECCOMP_H
#define SECCOMP_H

#include "types.h"

/* Seccomp modes */
#define SECCOMP_MODE_DISABLED 0
#define SECCOMP_MODE_STRICT   1
#define SECCOMP_MODE_FILTER   2

/* Seccomp filter */
typedef struct seccomp_filter {
    u32 mode;
    void* filter;
    size_t filter_size;
    struct seccomp_filter* next;
} seccomp_filter_t;

/* Initialize seccomp */
void seccomp_init(void);

/* Set seccomp mode */
int seccomp_set_mode_strict(void);
int seccomp_set_mode_filter(void* filter, size_t size);

/* Get seccomp mode */
int seccomp_get_mode(void);

/* Check if syscall allowed */
bool seccomp_check_syscall(u64 syscall_num);

/* Cleanup seccomp filter for process */
void seccomp_cleanup(process_t* proc);

#endif /* SECCOMP_H */
