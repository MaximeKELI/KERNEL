#include "workqueue.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

workqueue_t* system_wq = NULL;

void workqueue_init(void) {
    system_wq = workqueue_create("system");
    DEBUG_INFO("Workqueue system initialized");
}

workqueue_t* workqueue_create(const char* name) {
    workqueue_t* wq = (workqueue_t*)kzalloc(sizeof(workqueue_t));
    if (!wq) {
        DEBUG_ERROR("Failed to allocate workqueue");
        return NULL;
    }
    
    strncpy(wq->name, name, sizeof(wq->name) - 1);
    spinlock_init(&wq->lock);
    wq->running = true;
    wq->work_list = NULL;
    
    DEBUG_INFO("Workqueue created: %s", name);
    return wq;
}

void workqueue_destroy(workqueue_t* wq) {
    if (!wq) return;
    
    spinlock_lock(&wq->lock);
    wq->running = false;
    
    /* Process remaining work */
    work_t* work = wq->work_list;
    while (work) {
        work_t* next = work->next;
        kfree(work);
        work = next;
    }
    
    spinlock_unlock(&wq->lock);
    kfree(wq);
}

work_t* work_create(void (*function)(void*), void* data) {
    work_t* work = (work_t*)kzalloc(sizeof(work_t));
    if (!work) {
        DEBUG_ERROR("Failed to allocate work");
        return NULL;
    }
    
    work->function = function;
    work->data = data;
    work->pending = true;
    work->next = NULL;
    
    return work;
}

int workqueue_queue_work(workqueue_t* wq, work_t* work) {
    if (!wq || !work) return -1;
    
    spinlock_lock(&wq->lock);
    
    if (!wq->running) {
        spinlock_unlock(&wq->lock);
        return -1;
    }
    
    work->next = wq->work_list;
    wq->work_list = work;
    
    spinlock_unlock(&wq->lock);
    
    return 0;
}

void workqueue_process(workqueue_t* wq) {
    if (!wq) return;
    
    spinlock_lock(&wq->lock);
    
    work_t* work = wq->work_list;
    work_t* prev = NULL;
    
    while (work) {
        if (work->pending) {
            work->pending = false;
            spinlock_unlock(&wq->lock);
            
            /* Execute work */
            if (work->function) {
                work->function(work->data);
            }
            
            spinlock_lock(&wq->lock);
            
            /* Remove completed work */
            if (prev) {
                prev->next = work->next;
            } else {
                wq->work_list = work->next;
            }
            
            work_t* to_free = work;
            work = work->next;
            kfree(to_free);
        } else {
            prev = work;
            work = work->next;
        }
    }
    
    spinlock_unlock(&wq->lock);
}
