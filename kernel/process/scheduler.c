#include "process.h"
#include "memory.h"
#include "stdio.h"
#include "interrupt.h"
#include "refcount.h"
#include "validate.h"
#include "overflow.h"

process_t* process_list = NULL;
static process_t* current_process = NULL;
u64 next_pid = 1;

void process_init(void) {
    printk("Scheduler: Initialized\n");
}

process_t* process_create(void* entry_point, u64 stack_size) {
    VALIDATE_PTR_RET(entry_point, NULL);
    VALIDATE_RANGE(stack_size, PAGE_SIZE, 64 * 1024 * 1024); /* 4KB to 64MB */
    
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (!proc) return NULL;
    
    /* Initialize refcount */
    proc->refcount = REFCOUNT_INIT;
    refcount_get(&proc->refcount); /* Initial reference */
    
    /* Check for overflow in page calculation */
    size_t pages_needed;
    size_t total_pages = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
    CHECK_ADD_OVERFLOW(total_pages, 0, &pages_needed);
    
    /* Allocate stack */
    void* stack = vmm_alloc_pages(pages_needed);
    if (!stack) {
        kfree(proc);
        return NULL;
    }
    
    /* Initialize process */
    proc->pid = next_pid++;
    proc->parent_pid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_READY;
    proc->stack_base = stack;
    proc->stack_size = stack_size;
    
    /* Check for overflow in stack pointer calculation */
    size_t stack_offset;
    CHECK_SUB_OVERFLOW(stack_size, 16, &stack_offset);
    proc->rsp = (u64)stack + stack_offset;
    proc->rbp = proc->rsp;
    proc->rip = (u64)entry_point;
    proc->rflags = 0x202;  /* Interrupts enabled */
    proc->cr3 = 0;  /* Use current page table */
    proc->priority = 0;
    proc->time_slice = 100;
    proc->runtime = 0;
    proc->files = NULL;
    proc->next = NULL;
    
    /* Setup stack for entry */
    u64* stack_top = (u64*)(proc->rsp);
    *stack_top = 0;  /* Return address (will cause fault if returns) */
    
    /* Add to process list */
    proc->next = process_list;
    process_list = proc;
    
    return proc;
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

void schedule(void) {
    if (!process_list) return;
    
    process_t* next = NULL;
    process_t* prev = NULL;
    
    /* Round-robin: find next ready process */
    if (current_process) {
        process_t* p = current_process->next;
        while (p != current_process) {
            if (!p) p = process_list;
            if (p->state == PROCESS_READY) {
                next = p;
                break;
            }
            p = p->next;
        }
    }
    
    if (!next) {
        /* Find first ready process */
        process_t* p = process_list;
        while (p) {
            if (p->state == PROCESS_READY) {
                next = p;
                break;
            }
            p = p->next;
        }
    }
    
    if (next && next != current_process) {
        process_t* old = current_process;
        current_process = next;
        current_process->state = PROCESS_RUNNING;
        if (old) {
            old->state = PROCESS_READY;
        }
        context_switch(old, current_process);
    }
}

void context_switch(process_t* from, process_t* to) {
    if (!to) return;
    
    /* Save from context */
    if (from) {
        __asm__ __volatile__(
            "mov %%rsp, %0\n\t"
            "mov %%rbp, %1"
            : "=m"(from->rsp), "=m"(from->rbp)
        );
    }
    
    /* Load to context */
    __asm__ __volatile__(
        "mov %0, %%rsp\n\t"
        "mov %1, %%rbp\n\t"
        "mov %2, %%cr3"
        :
        : "r"(to->rsp), "r"(to->rbp), "r"(to->cr3)
        : "memory"
    );
    
    /* Jump to process */
    __asm__ __volatile__("jmp *%0" : : "r"(to->rip));
}

void yield(void) {
    if (current_process) {
        current_process->state = PROCESS_READY;
    }
    schedule();
}

void idle_task(void) {
    while (true) {
        __asm__ __volatile__("hlt");
        schedule();
    }
}
