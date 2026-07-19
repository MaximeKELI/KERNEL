#include "process.h"
#include "scheduler.h"
#include "exec.h"
#include "memory.h"
#include "mm.h"
#include "trapframe.h"
#include "fs/vfs.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "interrupt.h"
#include "validate.h"

extern process_t* process_list;
extern u64 next_pid;
extern void fork_child_trampoline(void);

#define KSTACK_SIZE (32 * 1024)

/*
 * fork(): create a child that resumes in ring 3 at the instruction right after
 * the parent's SYSCALL, returning 0, in its own COW copy of the parent's
 * address space, with an inherited copy of the file-descriptor table.
 *
 * The mechanism:
 *   - vmm_fork_clone() clones the caller's CR3 with copy-on-write user pages.
 *   - mm_clone() duplicates the VMA description so the child's faults resolve.
 *   - a fresh kernel stack is laid out with (top-to-bottom) a fork_frame_t that
 *     iretq's to ring 3, then a switch_to frame that lands in
 *     fork_child_trampoline with r15 -> the fork_frame. The first time the
 *     scheduler switches to the child, it unwinds straight back to userspace.
 */
process_t* fork_process(void) {
    process_t* parent = process_current();
    VALIDATE_PTR_RET(parent, NULL);

    const syscall_frame_t* sf = (const syscall_frame_t*)parent->syscall_regs;
    if (!sf) {
        DEBUG_ERROR("%s", "fork outside a syscall context");
        return NULL;
    }

    process_t* child = (process_t*)kzalloc(sizeof(process_t));
    if (!child) {
        return NULL;
    }

    /* Fresh kernel stack for the child (never shared with the parent). */
    void* kstack = vmm_alloc_pages(KSTACK_SIZE / PAGE_SIZE);
    if (!kstack) {
        kfree(child);
        return NULL;
    }

    /* Copy of the parent's user address space (COW). */
    if (parent->cr3 == 0) {
        parent->cr3 = vmm_get_cr3();
    }
    u64 child_cr3 = vmm_fork_clone();
    if (child_cr3 == 0) {
        vmm_free_pages(kstack, KSTACK_SIZE / PAGE_SIZE);
        kfree(child);
        return NULL;
    }

    /* Inherit process identity/credentials but start fresh scheduling state. */
    child->pid = 0;                       /* assigned below */
    child->parent_pid = parent->pid;
    child->state = PROCESS_READY;
    child->cr3 = child_cr3;
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->priority = parent->priority;
    child->time_slice = parent->time_slice;
    child->stack_base = kstack;
    child->stack_size = KSTACK_SIZE;
    child->refcount.count = 1;
    spinlock_init(&child->refcount.lock);
    spinlock_init(&child->lock);
    child->private_data = NULL;           /* signals are not inherited (P4) */
    child->syscall_regs = NULL;
    child->sched_node.proc = child;
    child->sched_node.on_rq = 0;
    child->exit_wq.head = NULL;
    child->wait_next = NULL;

    /* Duplicate the VMA map and the fd table. */
    child->mm = parent->mm ? mm_clone(parent->mm) : NULL;
    child->files = files_clone(parent->files);

    /* Build the child's ring-3 resume image at the top of its kernel stack. */
    u64 top = ((u64)kstack + KSTACK_SIZE) & ~0xFULL;
    fork_frame_t* ff = (fork_frame_t*)(top - sizeof(fork_frame_t));
    ff->rax = 0;                          /* child's fork() return value */
    ff->rbx = sf->rbx;
    ff->rcx = 0;
    ff->rdx = sf->rdx;
    ff->rsi = sf->rsi;
    ff->rdi = sf->rdi;
    ff->rbp = sf->rbp;
    ff->r8 = sf->r8;
    ff->r9 = sf->r9;
    ff->r10 = sf->r10;
    ff->r11 = 0;
    ff->r12 = sf->r12;
    ff->r13 = sf->r13;
    ff->r14 = sf->r14;
    ff->r15 = sf->r15;
    ff->rip = sf->user_rip;
    ff->cs = 0x23;                        /* user code selector (RPL 3) */
    ff->rflags = sf->user_rflags | 0x202; /* keep IF set */
    ff->rsp = sf->user_rsp;
    ff->ss = 0x2b;                        /* user data selector (RPL 3) */

    /* switch_to() frame: pop r15,r14,r13,r12,rbx,rbp then ret. We want r15 to
     * hold &fork_frame and the return to land in fork_child_trampoline. */
    u64* sp = (u64*)((u64)ff - 7 * 8);
    sp[0] = (u64)ff;                      /* -> r15 */
    sp[1] = 0;                            /* r14 */
    sp[2] = 0;                            /* r13 */
    sp[3] = 0;                            /* r12 */
    sp[4] = 0;                            /* rbx */
    sp[5] = 0;                            /* rbp */
    sp[6] = (u64)fork_child_trampoline;   /* return address */
    child->rsp = (u64)sp;
    child->rbp = 0;
    child->rip = sf->user_rip;
    child->rflags = ff->rflags;

    u64 flags = local_irq_save();
    child->next = process_list;
    process_list = child;
    child->pid = next_pid++;
    local_irq_restore(flags);

    DEBUG_INFO("Forked process: parent=%u, child=%u", (u32)parent->pid, (u32)child->pid);

    /* Make the child runnable. */
    wake_up_process(child);
    return child;
}

/* Exec - replace process image */
int exec_process(const char* path, char* const argv[]) {
    process_t* proc = process_current();
    VALIDATE_PTR(proc);

    VALIDATE_STRING(path, 4096);
    if (argv) {
        for (int i = 0; i < 64 && argv[i]; i++) {
            VALIDATE_STRING(argv[i], 4096);
        }
    }

    if (exec_run_path_argv(path, argv) < 0) {
        DEBUG_ERROR("Exec failed: %s", path);
        return -1;
    }
    return 0;
}

/*
 * wait(): reap a zombie child and return its pid + exit status. Blocks until a
 * child exits (the exit path wakes us via process_notify_parent_exit). Returns
 * -1 immediately if the caller has no matching children.
 */
int wait_process(u64 pid, int* status) {
    process_t* parent = process_current();
    VALIDATE_PTR(parent);

    for (;;) {
        u64 flags = local_irq_save();

        process_t* found = NULL;
        bool have_children = false;
        for (process_t* c = process_list; c; c = c->next) {
            if (c->parent_pid != parent->pid) {
                continue;
            }
            if (pid != 0 && c->pid != pid) {
                continue;
            }
            have_children = true;
            if (c->state == PROCESS_ZOMBIE || c->state == PROCESS_DEAD) {
                found = c;
                break;
            }
        }

        if (found) {
            u64 cpid = found->pid;
            if (status) {
                *status = found->exit_status;
            }
            local_irq_restore(flags);
            process_reap(found);
            return (int)cpid;
        }

        if (!have_children) {
            local_irq_restore(flags);
            return -1;
        }

        /* Block until a child exits. IRQs stay masked from the scan through the
         * state change so a child cannot exit-and-wake between them (single CPU:
         * a child only runs once we schedule). */
        parent->state = PROCESS_BLOCKED;
        schedule();
        local_irq_restore(flags);
    }
}
