#ifndef KTHREAD_H
#define KTHREAD_H

#include "types.h"

/* Kernel thread structure */
typedef struct kthread {
    u64 tid;
    void (*function)(void*);
    void* data;
    u64 stack;
    u64 rsp;
    bool running;
    struct kthread* next;
} kthread_t;

/* Initialize kernel threads */
void kthread_init(void);

/* Create kernel thread */
kthread_t* kthread_create(void (*function)(void*), void* data, const char* name);

/* Start kernel thread */
int kthread_start(kthread_t* thread);

/* Stop kernel thread */
int kthread_stop(kthread_t* thread);

/* Yield kernel thread */
void kthread_yield(void);

/* Get current kernel thread */
kthread_t* kthread_current(void);

#endif /* KTHREAD_H */
