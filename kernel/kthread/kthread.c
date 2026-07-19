#include "kthread.h"
#include "scheduler.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "interrupt.h"

#define KTHREAD_STACK_SIZE (4 * PAGE_SIZE)

static kthread_t* kthread_list = NULL;
static kthread_t* current_kthread = NULL;
static spinlock_t kthread_lock = SPINLOCK_INIT;
static u64 next_tid = 1;

void kthread_init(void) {
    DEBUG_INFO("Kernel threads initialized");
}

kthread_t* kthread_create(void (*function)(void*), void* data, const char* name) {
    (void)name;
    
    kthread_t* thread = (kthread_t*)kzalloc(sizeof(kthread_t));
    if (!thread) {
        DEBUG_ERROR("Failed to allocate kernel thread");
        return NULL;
    }
    
    thread->stack = (u64)vmm_alloc_pages((KTHREAD_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE);
    if (!thread->stack) {
        DEBUG_ERROR("Failed to allocate kernel thread stack");
        kfree(thread);
        return NULL;
    }
    
    thread->tid = next_tid++;
    thread->function = function;
    thread->data = data;
    thread->rsp = thread->stack + KTHREAD_STACK_SIZE - 16;
    thread->running = false;
    
    /* Setup stack for function call */
    u64* stack = (u64*)(thread->rsp);
    *stack = 0; /* Return address */
    
    spinlock_lock(&kthread_lock);
    thread->next = kthread_list;
    kthread_list = thread;
    spinlock_unlock(&kthread_lock);
    
    DEBUG_INFO("Kernel thread created: tid=%u", (u32)thread->tid);
    return thread;
}

int kthread_start(kthread_t* thread) {
    if (!thread) return -1;
    
    spinlock_lock(&kthread_lock);
    if (thread->running) {
        spinlock_unlock(&kthread_lock);
        return -1;
    }
    
    thread->running = true;
    spinlock_unlock(&kthread_lock);
    
    /* Would switch to thread here */
    DEBUG_INFO("Kernel thread started: tid=%u", (u32)thread->tid);
    return 0;
}

int kthread_stop(kthread_t* thread) {
    if (!thread) return -1;
    
    spinlock_lock(&kthread_lock);
    thread->running = false;
    spinlock_unlock(&kthread_lock);
    
    DEBUG_INFO("Kernel thread stopped: tid=%u", (u32)thread->tid);
    return 0;
}

void kthread_yield(void) {
    /* Yield to next kernel thread */
    schedule();
}

kthread_t* kthread_current(void) {
    return current_kthread;
}
