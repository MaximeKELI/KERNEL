#include "syscall.h"
#include "process.h"
#include "scheduler.h"
#include "stdio.h"
#include "fs/vfs.h"
#include "io.h"
#include "validate.h"
#include "seccomp.h"
#include "dns.h"
#include "net_addr.h"
#include "uaccess.h"
#include "exec.h"
#include "ai_manager.h"
#include "landlock.h"
#include "audit.h"
#include "memory.h"
#include "mm.h"
#include "signal.h"
#include "trapframe.h"
#include "kspp.h"
#include "fb_console.h"
#include "gdt.h"
#include "epoll.h"
#include "net.h"
#include "vdso.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"

extern int socket_fd_poll_events(int fd);

typedef u64 (*syscall_func_t)(u64, u64, u64, u64, u64);

static syscall_func_t syscall_table[] = {
    (syscall_func_t)sys_exit,
    (syscall_func_t)sys_write,
    (syscall_func_t)sys_read,
    (syscall_func_t)sys_open,
    (syscall_func_t)sys_close,
    (syscall_func_t)sys_fork,
    (syscall_func_t)sys_exec,
    (syscall_func_t)sys_wait,
    (syscall_func_t)sys_mmap,
    (syscall_func_t)sys_munmap,
    (syscall_func_t)sys_rt_sigreturn,
    (syscall_func_t)sys_socket,
    (syscall_func_t)sys_bind,
    (syscall_func_t)sys_connect,
    (syscall_func_t)sys_listen,
    (syscall_func_t)sys_accept,
    (syscall_func_t)sys_send,
    (syscall_func_t)sys_recv,
    (syscall_func_t)sys_sendto,
    (syscall_func_t)sys_socket_close,
    (syscall_func_t)sys_dns_resolve,
    (syscall_func_t)sys_ai_metrics,
    (syscall_func_t)sys_epoll_create,
    (syscall_func_t)sys_epoll_ctl,
    (syscall_func_t)sys_epoll_wait,
    (syscall_func_t)sys_poll,
    (syscall_func_t)sys_getpid,
    (syscall_func_t)sys_clock_gettime,
    (syscall_func_t)sys_brk,
    (syscall_func_t)sys_mprotect,
    (syscall_func_t)sys_rt_sigaction,
    (syscall_func_t)sys_rt_sigprocmask,
    (syscall_func_t)sys_kill,
};

static char audit_buf[64];

u64 syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5,
                    void* uframe) {
    if (syscall_num >= sizeof(syscall_table) / sizeof(syscall_table[0])) {
        DEBUG_ERROR("Invalid syscall number: %llu", (unsigned long long)syscall_num);
        return (u64)-1;
    }

    process_t* proc = process_current();
    if (proc) {
        /* Remember the caller's user register block so fork() can clone it. */
        proc->syscall_regs = uframe;
    }
    if (proc && !seccomp_check_syscall(syscall_num)) {
        snprintf(audit_buf, sizeof(audit_buf), "deny syscall %llu", (unsigned long long)syscall_num);
        audit_log(AUDIT_SYSCALL, audit_buf);
        return (u64)-1;
    }

    if (!kspp_stack_check()) {
        audit_log(AUDIT_SYSCALL, "stack canary fault");
        return (u64)-1;
    }

    syscall_func_t func = syscall_table[syscall_num];
    if (!func) {
        return (u64)-1;
    }

    u64 ret = func(arg1, arg2, arg3, arg4, arg5);

    /*
     * The fork() child does NOT return through this path: it resumes directly
     * in ring 3 via fork_child_trampoline with rax already 0. Only the parent
     * runs here, and sys_fork() returns the child pid.
     */

    /* Deliver a pending signal on the way back to user mode. Uses the caller's
     * saved register block so the handler runs in ring 3 and sigreturn resumes
     * the interrupted code (or the syscall's result). */
    if (proc && proc->syscall_regs && signal_has_pending(proc)) {
        syscall_frame_t* sf = (syscall_frame_t*)proc->syscall_regs;
        sigcontext_t ctx;
        ctx.rax = ret; ctx.rbx = sf->rbx; ctx.rcx = 0; ctx.rdx = sf->rdx;
        ctx.rsi = sf->rsi; ctx.rdi = sf->rdi; ctx.rbp = sf->rbp;
        ctx.r8 = sf->r8; ctx.r9 = sf->r9; ctx.r10 = sf->r10; ctx.r11 = 0;
        ctx.r12 = sf->r12; ctx.r13 = sf->r13; ctx.r14 = sf->r14; ctx.r15 = sf->r15;
        ctx.rip = sf->user_rip; ctx.rsp = sf->user_rsp; ctx.rflags = sf->user_rflags;
        if (signal_dispatch(proc, &ctx, true) > 0) {
            sf->user_rip = ctx.rip;
            sf->user_rsp = ctx.rsp;
            sf->user_rflags = ctx.rflags;
            sf->rdi = ctx.rdi;    /* signo -> handler's first argument */
        }
    }

    if (syscall_num == SYS_OPEN || syscall_num == SYS_EXEC) {
        snprintf(audit_buf, sizeof(audit_buf), "syscall %llu ret=%llu",
                 (unsigned long long)syscall_num, (unsigned long long)ret);
        audit_log(AUDIT_SYSCALL, audit_buf);
    }

    return ret;
}

void syscall_init(void) {
    extern void syscall_entry(void);
    gdt_init_user_segments();
    syscall_socket_init();
    u64 efer = rdmsr(0xC0000080);
    wrmsr(0xC0000080, efer | (1 << 0));
    /*
     * STAR (Intel SDM):
     *   [47:32] SYSCALL loads CS = this selector, SS = this+8.  -> kernel 0x08/0x10
     *   [63:48] SYSRET base: CS = base+16, SS = base+8 (both forced RPL 3).
     *           base 0x10 => CS=0x23 (user code), SS=0x1B (user data).
     * Getting [47:32] wrong makes SYSCALL run the kernel with a user-code CS
     * (CS=0x20) and blows up on the first privileged operation.
     */
    wrmsr(0xC0000081, ((u64)0x10 << 48) | ((u64)GDT_KERNEL_CODE << 32));
    wrmsr(0xC0000082, (u64)syscall_entry);
    wrmsr(0xC0000084, 0x200);
    printk("Syscall: SYSCALL/SYSRET ring0/ring3\n");
}

u64 sys_exit(u64 status) {
    /*
     * Terminate the current task the same way a kernel thread does: record the
     * exit code, wake any thread_join()/wait waiters and switch away for good.
     * kthread_exit() never returns.
     */
    kthread_exit((int)status);
    return 0;
}

u64 sys_write(u64 fd, const void* buf, u64 count) {
    VALIDATE_RANGE(fd, 0, 255);
    VALIDATE_PTR_RET(buf, 0);
    VALIDATE_RANGE(count, 0, 1024 * 1024);

    if (fd == 1 || fd == 2) {
        const char* str = (const char*)buf;
        for (u64 i = 0; i < count; i++) {
            printk("%c", str[i]);
        }
        fb_console_write(str, count);
        return count;
    }
    return (u64)vfs_write_fd((int)fd, buf, (size_t)count);
}

u64 sys_read(u64 fd, void* buf, u64 count) {
    VALIDATE_PTR_RET(buf, 0);
    if (fd == 0) {
        /* stdin: block on the keyboard line discipline, then hand the line to
         * the user buffer. Lets ring-3 shells read commands interactively. */
        if (count == 0) {
            return 0;
        }
        char line[256];
        u32 max = count < sizeof(line) ? (u32)count : (u32)sizeof(line);
        u32 n = keyboard_read_line(line, max);
        if (copy_to_user(buf, line, n) < 0) {
            return (u64)-1;
        }
        return n;
    }
    if (fd < 3) {
        return 0;
    }
    return (u64)vfs_read_fd((int)fd, buf, (size_t)count);
}

u64 sys_open(const char* path, u64 flags) {
    char kpath[256];
    if (!path) {
        return (u64)-1;
    }
    if (copy_from_user(kpath, path, sizeof(kpath) - 1) < 0) {
        return (u64)-1;
    }
    kpath[sizeof(kpath) - 1] = '\0';

    process_t* proc = process_current();
    u64 pid = proc ? proc->pid : 0;
    if (!landlock_check_access(pid, kpath, LANDLOCK_ACCESS_FS_READ)) {
        audit_log(AUDIT_PATH, "landlock deny open");
        return (u64)-1;
    }

    int fd = vfs_open_fd(kpath, flags);
    return fd < 0 ? (u64)-1 : (u64)fd;
}

u64 sys_close(u64 fd) {
    if (fd >= 3 && fd < 256) {
        return vfs_close_fd((int)fd) == 0 ? 0 : (u64)-1;
    }
    if (fd >= 3) {
        return sys_socket_close(fd);
    }
    return 0;
}

u64 sys_fork(void) {
    process_t* parent = process_current();
    if (!parent) {
        return (u64)-1;
    }
    process_t* child = fork_process();
    if (!child) {
        return (u64)-1;
    }
    return child->pid;
}

u64 sys_getpid(void) {
    process_t* proc = process_current();
    return proc ? proc->pid : (u64)-1;
}

typedef struct timespec_user {
    i64 tv_sec;
    i64 tv_nsec;
} timespec_user_t;

u64 sys_clock_gettime(u64 clk_id, void* tp) {
    (void)clk_id;
    if (!tp) {
        return (u64)-1;
    }
    timespec_user_t ts;
    u64 ticks = timer_get_ticks();
    ts.tv_sec = (i64)(ticks / 1000);
    ts.tv_nsec = (i64)((ticks % 1000) * 1000000);
    if (copy_to_user(tp, &ts, sizeof(ts)) < 0) {
        return (u64)-1;
    }
    return 0;
}

/* Bounded kernel copies of the argv strings passed to execve(). */
#define EXEC_MAX_ARGS 16
#define EXEC_ARG_LEN  128
static char exec_arg_store[EXEC_MAX_ARGS][EXEC_ARG_LEN];
static char* exec_argv[EXEC_MAX_ARGS + 1];

u64 sys_exec(const char* path, char* const argv[]) {
    char kpath[256];
    if (!path) {
        return (u64)-1;
    }
    if (copy_from_user(kpath, path, sizeof(kpath) - 1) < 0) {
        return (u64)-1;
    }
    kpath[sizeof(kpath) - 1] = '\0';

    /*
     * Snapshot argv into kernel memory BEFORE exec tears down the user address
     * space (the argv pointers/strings live in the caller's memory, which is
     * about to be unmapped).
     */
    char* const* kargv = NULL;
    if (argv) {
        int argc = 0;
        for (; argc < EXEC_MAX_ARGS; argc++) {
            char* uptr = NULL;
            if (copy_from_user(&uptr, (const char*)&argv[argc], sizeof(uptr)) < 0) {
                break;
            }
            if (!uptr) {
                break;
            }
            if (copy_from_user(exec_arg_store[argc], uptr, EXEC_ARG_LEN - 1) < 0) {
                break;
            }
            exec_arg_store[argc][EXEC_ARG_LEN - 1] = '\0';
            exec_argv[argc] = exec_arg_store[argc];
        }
        exec_argv[argc] = NULL;
        kargv = exec_argv;
    }

    seccomp_set_mode_strict();

    if (exec_run_path_argv(kpath, kargv) < 0) {
        return (u64)-1;
    }
    return 0;
}

u64 sys_wait(u64 pid, int* status_user) {
    int status = 0;
    int r = wait_process(pid, &status);
    if (r < 0) {
        return (u64)-1;
    }
    if (status_user) {
        /* Encode like Linux wait status: exit code in bits 8..15. */
        int wstatus = (status & 0xff) << 8;
        copy_to_user(status_user, &wstatus, sizeof(wstatus));
    }
    return (u64)r;
}

static u64 prot_to_vm(u64 prot) {
    u64 vm = 0;
    if (prot & PROT_READ)  vm |= VM_READ;
    if (prot & PROT_WRITE) vm |= VM_WRITE;
    if (prot & PROT_EXEC)  vm |= VM_EXEC;
    return vm;
}

/* Anonymous memory only for now; backing is demand-paged by mm_fault(). */
u64 sys_mmap(void* addr, u64 length, u64 prot, u64 flags) {
    (void)flags;
    process_t* proc = process_current();
    if (!proc || !proc->mm || length == 0) {
        return (u64)-1;
    }
    u64 vm = prot_to_vm(prot);
    if (vm == 0) {
        vm = VM_READ;
    }
    return mm_mmap(proc->mm, (u64)addr, length, vm);
}

u64 sys_munmap(void* addr, u64 length) {
    process_t* proc = process_current();
    if (!proc || !proc->mm || !addr || length == 0) {
        return (u64)-1;
    }
    return (u64)mm_remove_range(proc->mm, (u64)addr, (u64)addr + length);
}

u64 sys_brk(u64 new_brk) {
    process_t* proc = process_current();
    if (!proc || !proc->mm) {
        return (u64)-1;
    }
    return mm_brk(proc->mm, new_brk);
}

u64 sys_mprotect(void* addr, u64 length, u64 prot) {
    process_t* proc = process_current();
    if (!proc || !proc->mm || !addr || length == 0) {
        return (u64)-1;
    }
    u64 vm = prot_to_vm(prot);
    return (u64)mm_protect_range(proc->mm, (u64)addr, (u64)addr + length, vm);
}

u64 sys_sigreturn(void) {
    return sys_rt_sigreturn();
}

u64 sys_rt_sigaction(u64 sig, const void* act, void* oldact) {
    sigaction_t kact, koldact;
    sigaction_t* pact = NULL;
    if (act) {
        if (copy_from_user(&kact, act, sizeof(kact)) < 0) {
            return (u64)-1;
        }
        pact = &kact;
    }
    int r = rt_sigaction((int)sig, pact, oldact ? &koldact : NULL);
    if (r == 0 && oldact) {
        if (copy_to_user(oldact, &koldact, sizeof(koldact)) < 0) {
            return (u64)-1;
        }
    }
    return r == 0 ? 0 : (u64)-1;
}

u64 sys_rt_sigprocmask(u64 how, const void* set, void* oldset) {
    sigset_t kset, koldset;
    sigset_t* pset = NULL;
    if (set) {
        if (copy_from_user(&kset, set, sizeof(kset)) < 0) {
            return (u64)-1;
        }
        pset = &kset;
    }
    int r = rt_sigprocmask((int)how, pset, oldset ? &koldset : NULL);
    if (r == 0 && oldset) {
        if (copy_to_user(oldset, &koldset, sizeof(koldset)) < 0) {
            return (u64)-1;
        }
    }
    return r == 0 ? 0 : (u64)-1;
}

u64 sys_kill(u64 pid, u64 sig) {
    return kill(pid, (int)sig) == 0 ? 0 : (u64)-1;
}

u64 sys_ai_metrics(void* out_info, u64 size) {
    if (!out_info || size < sizeof(ai_user_info_t)) {
        return (u64)-1;
    }
    if (!ai_initialized) {
        return (u64)-1;
    }
    ai_user_info_t info;
    ai_fill_user_info(&info);
    if (copy_to_user(out_info, &info, sizeof(info)) < 0) {
        return (u64)-1;
    }
    return 0;
}

u64 sys_epoll_create(u64 size) {
    int fd = epoll_create((int)size);
    return fd < 0 ? (u64)-1 : (u64)fd;
}

u64 sys_epoll_ctl(u64 epfd, u64 op, u64 fd, void* event) {
    epoll_event_t ev;
    if (event && copy_from_user(&ev, event, sizeof(ev)) < 0) {
        return (u64)-1;
    }
    return epoll_ctl((int)epfd, (int)op, (int)fd, event ? &ev : NULL) == 0 ? 0 : (u64)-1;
}

u64 sys_epoll_wait(u64 epfd, void* events, u64 maxevents, u64 timeout) {
    if (!events || maxevents == 0) {
        return (u64)-1;
    }
    epoll_event_t stack[16];
    u64 n = maxevents > 16 ? 16 : maxevents;
    int r = epoll_wait((int)epfd, stack, (int)n, (int)timeout);
    if (r < 0) {
        return (u64)-1;
    }
    if (copy_to_user(events, stack, (size_t)r * sizeof(epoll_event_t)) < 0) {
        return (u64)-1;
    }
    return (u64)r;
}

typedef struct pollfd_u {
    int fd;
    short events;
    short revents;
} pollfd_u_t;

u64 sys_poll(void* fds, u64 nfds, u64 timeout_ms) {
    if (!fds || nfds == 0) {
        return 0;
    }
    pollfd_u_t p;
    u64 ready = 0;
    for (u64 i = 0; i < nfds && i < 32; i++) {
        if (copy_from_user(&p, (char*)fds + i * sizeof(pollfd_u_t), sizeof(p)) < 0) {
            break;
        }
        p.revents = (short)socket_fd_poll_events(p.fd);
        if (p.revents) {
            ready++;
        }
        copy_to_user((char*)fds + i * sizeof(pollfd_u_t), &p, sizeof(p));
    }
    if (ready == 0 && timeout_ms > 0) {
        for (u64 t = 0; t < timeout_ms; t++) {
            net_poll();
        }
    }
    return ready;
}

u64 sys_dns_resolve(const char* hostname, void* out_ip, u64 out_len) {
    if (!hostname || !out_ip || out_len < 4) {
        return (u64)-1;
    }
    ip_addr_t ip;
    if (dns_resolve_a(hostname, &ip) < 0) {
        return (u64)-1;
    }
    if (copy_to_user(out_ip, ip.addr, 4) < 0) {
        return (u64)-1;
    }
    return 0;
}
