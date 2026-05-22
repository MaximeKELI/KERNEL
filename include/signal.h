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

/* Signal action */
typedef struct {
    sighandler_t handler;
    sigset_t mask;
    u32 flags;
} sigaction_t;

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

void signal_return_from_handler(void);
int signal_deliver_pending(process_t* proc);

#endif /* SIGNAL_H */
