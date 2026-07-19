#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

struct process;
typedef struct process process_t;

/* Signal numbers */
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22

#define NSIG 32

/* Signal actions */
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

/* Signal handler type */
typedef void (*sighandler_t)(int);

/* Signal set */
typedef struct {
    u64 sig[(NSIG + 63) / 64];  /* Ensure at least 1 element */
} sigset_t;

/* sigprocmask 'how' values (match Linux). */
#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

/* sigaction flags (subset). */
#define SA_RESTORER 0x04000000

/* Signal action */
typedef struct {
    sighandler_t handler;
    sigset_t mask;
    u32 flags;
} sigaction_t;

/*
 * Saved user register state for signal delivery. A copy is pushed onto the
 * user stack when a handler is invoked and restored verbatim by sigreturn, so
 * the interrupted code resumes exactly where it left off.
 */
typedef struct sigcontext {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp;
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
    u64 rip, rsp, rflags;
} sigcontext_t;

/* Initialize signal system */
void signal_init(void);

/* Send signal to process */
int kill(u64 pid, int sig);

/* Set signal handler */
sighandler_t signal(int sig, sighandler_t handler);

/* Block/unblock signals */
int sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

/* Wait for signal */
int sigwait(const sigset_t* set, int* sig);

/* POSIX rt_* variants (kernel-internal helpers behind the syscalls). */
int rt_sigaction(int sig, const sigaction_t* act, sigaction_t* oldact);
int rt_sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

/*
 * Core delivery. Given the interrupted user context in *ctx, deliver the first
 * pending, unblocked signal: push a signal frame on the user stack and rewrite
 * *ctx to enter the handler. Returns the delivered signo, 0 if none pending, or
 * performs the default action (which may terminate the task and never return).
 * `on_syscall` tweaks how rax is treated (syscall return value vs. live reg).
 */
int signal_dispatch(process_t* proc, sigcontext_t* ctx, bool on_syscall);

/* True if `proc` has a signal pending that is not currently blocked. */
bool signal_has_pending(process_t* proc);

/* Raise SIGSEGV/SIGILL/etc. on a faulting task (from the exception handler). */
void signal_force(process_t* proc, int sig);

/* fork(): inherit handlers + blocked mask (pending is not inherited). */
void* signal_clone(void* src);
/* Free a task's signal state (exit / exec-reset). */
void signal_state_free(void* st);

void signal_return_from_handler(void);   /* sigreturn body */
u64  sys_rt_sigreturn(void);

#endif /* SIGNAL_H */
