#include "process.h"
#include "scheduler.h"
#include "ai_types.h"
#include "memory.h"
#include "stdio.h"
#include "interrupt.h"
#include "refcount.h"
#include "validate.h"
#include "overflow.h"
#include "memory.h"

process_t* process_list = NULL;
process_t* current_process = NULL;   /* the task running on the (single) CPU */
u64 next_pid = 1;

/* First-run trampoline for kernel threads (kernel/asm/context_switch.S). */
extern void kthread_trampoline(void);

void process_init(void) {
    extern void vmm_init_user_mm(void);
    vmm_init_user_mm();
}

/*
 * Allocate a kernel thread and lay out its initial kernel stack so that the
 * very first switch_to() into it pops the six callee-saved slots and returns
 * into kthread_trampoline, with the entry point in r15 and the argument in r14.
 * The task is created READY but is NOT placed on the runqueue.
 */
static process_t* proc_create_kthread(void (*entry)(void*), void* arg, u64 stack_size) {
    if (!entry) {
        return NULL;
    }
    if (stack_size < PAGE_SIZE || stack_size > 64 * 1024 * 1024) {
        DEBUG_ERROR("Invalid stack size: %u", (u32)stack_size);
        return NULL;
    }

    process_t* proc = (process_t*)kzalloc(sizeof(process_t));
    if (!proc) return NULL;

    proc->refcount.count = 1;
    spinlock_init(&proc->refcount.lock);
    spinlock_init(&proc->lock);
    proc->private_data = NULL;

    size_t pages_needed = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
    void* stack = vmm_alloc_pages(pages_needed);
    if (!stack) {
        kfree(proc);
        return NULL;
    }

    proc->pid = next_pid++;
    proc->parent_pid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_READY;
    proc->stack_base = stack;
    proc->stack_size = pages_needed * PAGE_SIZE;
    proc->cr3 = vmm_get_cr3();          /* kernel threads share the kernel address space */
    proc->priority = PRIO_DEFAULT;
    proc->time_slice = 100;
    proc->time_slice_left = 0;
    proc->runtime = 0;
    proc->ai_class = AI_CLASS_IDLE;
    proc->files = NULL;
    proc->sched_node.proc = proc;
    proc->sched_node.on_rq = 0;

    /* Build the initial switch_to frame at the top of the stack. */
    u64 top = ((u64)stack + proc->stack_size) & ~0xFULL;   /* 16-aligned */
    u64* sp = (u64*)(top - 7 * 8);                          /* => sp % 16 == 8 */
    sp[0] = (u64)entry;                 /* popped into r15 */
    sp[1] = (u64)arg;                   /* popped into r14 */
    sp[2] = 0;                          /* r13 */
    sp[3] = 0;                          /* r12 */
    sp[4] = 0;                          /* rbx */
    sp[5] = 0;                          /* rbp */
    sp[6] = (u64)kthread_trampoline;    /* return address */
    proc->rsp = (u64)sp;
    proc->rbp = 0;
    proc->rip = (u64)entry;             /* informational only */
    proc->rflags = 0x202;

    u64 flags = local_irq_save();
    proc->next = process_list;
    process_list = proc;
    local_irq_restore(flags);

    return proc;
}

process_t* process_create(void* entry_point, u64 stack_size) {
    VALIDATE_PTR_RET(entry_point, NULL);
    /*
     * Created but intentionally NOT runnable: the caller must wake_up_process()
     * to schedule it. This preserves the old contract used by callers that pass
     * a placeholder entry point purely to test allocation.
     */
    return proc_create_kthread((void (*)(void*))entry_point, NULL, stack_size);
}

process_t* kthread_create_stopped(void (*entry)(void*), void* arg, u64 stack_size) {
    return proc_create_kthread(entry, arg, stack_size);
}

process_t* kthread_run(void (*entry)(void*), void* arg, u64 stack_size) {
    process_t* p = proc_create_kthread(entry, arg, stack_size);
    if (p) {
        wake_up_process(p);
    }
    return p;
}

void thread_exit(void) {
    process_t* p = current_process;
    if (p) {
        p->state = PROCESS_ZOMBIE;   /* a zombie is never re-enqueued by schedule() */
    }
    schedule();                      /* switch away for good */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

void process_destroy(process_t* proc) {
    VALIDATE_PTR_VOID(proc);
    
    /* Cleanup seccomp filter if present */
    extern void seccomp_cleanup(process_t*);
    seccomp_cleanup(proc);
    
    /* Free stack */
    if (proc->stack_base) {
        size_t pages = (proc->stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
        vmm_free_pages(proc->stack_base, pages);
    }
    
    /* Remove from list */
    if (process_list == proc) {
        process_list = proc->next;
    } else {
        process_t* p = process_list;
        while (p && p->next != proc) {
            p = p->next;
        }
        if (p) {
            p->next = proc->next;
        }
    }
    
    kfree(proc);
}

process_t* process_current(void) {
    return current_process;
}

void yield(void) {
    /*
     * Give up the CPU but stay runnable: schedule() requeues the current task
     * (it is still RUNNING) with an updated vruntime and picks the next one.
     */
    schedule();
}

void idle_task(void) {
    while (true) {
        __asm__ __volatile__("hlt");
        schedule();
    }
}

u64 scheduler_get_running_count(u32 cpu_id) {
    (void)cpu_id; /* TODO: Per-CPU tracking */
    
    u64 count = 0;
    process_t* p = process_list;
    
    while (p) {
        if (p->state == PROCESS_RUNNING) {
            count++;
        }
        p = p->next;
    }
    
    return count;
}
