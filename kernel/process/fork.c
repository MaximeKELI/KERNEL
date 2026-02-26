#include "process.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "interrupt.h"
#include "validate.h"
#include "overflow.h"

/* Fork process - create child copy */
process_t* fork_process(void) {
    process_t* parent = process_current();
    VALIDATE_PTR_RET(parent, NULL);
    
    /* Allocate new process */
    process_t* child = (process_t*)kmalloc(sizeof(process_t));
    if (!child) {
        DEBUG_ERROR("Failed to allocate child process");
        return NULL;
    }
    
    /* Copy parent process */
    memcpy(child, parent, sizeof(process_t));
    
    /* Validate stack size */
    VALIDATE_RANGE(parent->stack_size, PAGE_SIZE, 64 * 1024 * 1024); /* 4KB to 64MB */
    
    /* Allocate new stack with overflow check */
    size_t pages_needed;
    size_t stack_pages = (parent->stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
    CHECK_ADD_OVERFLOW(stack_pages, 0, &pages_needed);
    
    child->stack_base = vmm_alloc_pages(pages_needed);
    if (!child->stack_base) {
        DEBUG_ERROR("Failed to allocate child stack");
        kfree(child);
        return NULL;
    }
    
    /* Copy stack */
    memcpy(child->stack_base, parent->stack_base, parent->stack_size);
    
    /* Update child properties */
    child->pid = 0; /* Will be assigned */
    child->parent_pid = parent->pid;
    child->state = PROCESS_READY;
    child->rsp = (u64)child->stack_base + ((u64)parent->rsp - (u64)parent->stack_base);
    child->rbp = (u64)child->stack_base + ((u64)parent->rbp - (u64)parent->stack_base);
    
    /* Copy page directory (simplified - would need proper copy-on-write) */
    child->cr3 = parent->cr3;
    
    /* Add to process list */
    extern process_t* process_list;
    child->next = process_list;
    process_list = child;
    
    /* Assign PID */
    extern u64 next_pid;
    child->pid = next_pid++;
    
    DEBUG_INFO("Forked process: parent=%u, child=%u", parent->pid, child->pid);
    
    return child;
}

/* Exec - replace process image */
int exec_process(const char* path, char* const argv[]) {
    process_t* proc = process_current();
    VALIDATE_PTR(proc);
    
    /* Validate parameters */
    VALIDATE_STRING(path, 4096);
    /* argv can be NULL, but if not NULL, validate it */
    if (argv) {
        for (int i = 0; i < 64 && argv[i]; i++) {
            VALIDATE_STRING(argv[i], 4096);
        }
    }
    
    (void)path;
    (void)argv;
    
    /* Would load new executable here */
    /* For now, just return success */
    DEBUG_INFO("Exec process: %s", path);
    
    return 0;
}

/* Wait for child process */
int wait_process(u64 pid, int* status) {
    process_t* parent = process_current();
    if (!parent) {
        return -1;
    }
    
    /* Find child process */
    extern process_t* process_list;
    process_t* child = process_list;
    
    while (child) {
        if (child->parent_pid == parent->pid && (pid == 0 || child->pid == pid)) {
            if (child->state == PROCESS_ZOMBIE || child->state == PROCESS_DEAD) {
                if (status) {
                    *status = 0; /* Would get exit status */
                }
                
                /* Clean up child */
                process_destroy(child);
                return (int)child->pid;
            }
        }
        child = child->next;
    }
    
    /* No child ready, wait */
    parent->state = PROCESS_BLOCKED;
    schedule();
    
    return wait_process(pid, status);
}
