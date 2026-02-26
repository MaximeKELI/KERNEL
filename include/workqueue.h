#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include "types.h"

/* Work item */
typedef struct work {
    void (*function)(void*);
    void* data;
    struct work* next;
    bool pending;
} work_t;

/* Work queue */
typedef struct workqueue {
    char name[32];
    work_t* work_list;
    spinlock_t lock;
    bool running;
} workqueue_t;

/* Initialize workqueues */
void workqueue_init(void);

/* Create workqueue */
workqueue_t* workqueue_create(const char* name);

/* Destroy workqueue */
void workqueue_destroy(workqueue_t* wq);

/* Queue work */
int workqueue_queue_work(workqueue_t* wq, work_t* work);

/* Create work */
work_t* work_create(void (*function)(void*), void* data);

/* Process workqueue */
void workqueue_process(workqueue_t* wq);

#endif /* WORKQUEUE_H */
