#include "signal.h"
#include "process.h"
#include "scheduler.h"
#include "stdio.h"
#include "debug.h"
#include "memory.h"
#include "string.h"
#include "interrupt.h"

/*
 * Per-process signal state. Lives in process_t.signal_state (a dedicated slot,
 * no longer aliasing private_data), so signals cannot clobber namespaces /
 * affinity / cgroups any more.
 */
typedef struct signal_state {
    sigaction_t actions[NSIG];
    sigset_t pending;
    sigset_t blocked;
} signal_state_t;

static inline void sigset_add(sigset_t* s, int sig) {
    s->sig[sig / 64] |= (1ULL << (sig % 64));
}
static inline void sigset_del(sigset_t* s, int sig) {
    s->sig[sig / 64] &= ~(1ULL << (sig % 64));
}
static inline bool sigset_test(const sigset_t* s, int sig) {
    return (s->sig[sig / 64] & (1ULL << (sig % 64))) != 0;
}

signal_state_t* signal_state_get(process_t* proc) {
    if (!proc) {
        return NULL;
    }
    signal_state_t* st = (signal_state_t*)proc->signal_state;
    if (!st) {
        st = (signal_state_t*)kzalloc(sizeof(signal_state_t));
        if (!st) {
            return NULL;
        }
        proc->signal_state = st;
    }
    return st;
}

void signal_init(void) {
    DEBUG_INFO("Signal system initialized");
}

void* signal_clone(void* src) {
    if (!src) {
        return NULL;
    }
    signal_state_t* s = (signal_state_t*)src;
    signal_state_t* d = (signal_state_t*)kzalloc(sizeof(signal_state_t));
    if (!d) {
        return NULL;
    }
    for (int i = 0; i < NSIG; i++) {
        d->actions[i] = s->actions[i];
    }
    d->blocked = s->blocked;
    /* pending signals are NOT inherited across fork */
    return d;
}

void signal_state_free(void* st) {
    if (st) {
        kfree(st);
    }
}

/* Default action: 1 = terminate, 0 = ignore. */
static int sig_default_terminates(int sig) {
    /* Ignored by default: SIGCHLD, SIGCONT (stop/cont not modelled yet). */
    if (sig == SIGCHLD || sig == SIGCONT) {
        return 0;
    }
    return 1;   /* everything else defaults to terminate */
}

static int do_send(process_t* proc, int sig) {
    if (!proc || sig < 1 || sig >= NSIG) {
        return -1;
    }
    signal_state_t* st = signal_state_get(proc);
    if (!st) {
        return -1;
    }
    sigset_add(&st->pending, sig);

    /* SIGKILL/SIGSTOP can neither be caught nor blocked. Terminate directly if
     * the target is not the running task (which will notice on its next return
     * to user mode otherwise). */
    if (sig == SIGKILL) {
        if (proc != process_current()) {
            proc->exit_status = 128 + sig;
            proc->state = PROCESS_ZOMBIE;
            process_notify_parent_exit(proc);
        }
    }
    /* Nudge a blocked target so it re-evaluates on its return path. */
    if (proc->state == PROCESS_BLOCKED) {
        wake_up_process(proc);
    }
    return 0;
}

int kill(u64 pid, int sig) {
    if (sig < 0 || sig >= NSIG) {
        return -1;
    }
    extern process_t* process_list;
    for (process_t* p = process_list; p; p = p->next) {
        if (p->pid == pid) {
            if (sig == 0) {
                return 0;   /* existence check only */
            }
            return do_send(p, sig);
        }
    }
    return -1;
}

void signal_force(process_t* proc, int sig) {
    if (!proc) {
        return;
    }
    signal_state_t* st = signal_state_get(proc);
    if (st) {
        /* A forced signal cannot be blocked (SIGSEGV on a bad access, etc.). */
        sigset_del(&st->blocked, sig);
    }
    do_send(proc, sig);
}

sighandler_t signal(int sig, sighandler_t handler) {
    if (sig < 1 || sig >= NSIG) {
        return SIG_ERR;
    }
    process_t* proc = process_current();
    signal_state_t* st = signal_state_get(proc);
    if (!st) {
        return SIG_ERR;
    }
    sighandler_t old = st->actions[sig].handler;
    st->actions[sig].handler = handler;
    return old;
}

int rt_sigaction(int sig, const sigaction_t* act, sigaction_t* oldact) {
    if (sig < 1 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) {
        return -1;
    }
    process_t* proc = process_current();
    signal_state_t* st = signal_state_get(proc);
    if (!st) {
        return -1;
    }
    if (oldact) {
        *oldact = st->actions[sig];
    }
    if (act) {
        st->actions[sig] = *act;
    }
    return 0;
}

int rt_sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    process_t* proc = process_current();
    signal_state_t* st = signal_state_get(proc);
    if (!st) {
        return -1;
    }
    if (oldset) {
        *oldset = st->blocked;
    }
    if (set) {
        for (int i = 0; i < (NSIG + 63) / 64; i++) {
            switch (how) {
            case SIG_BLOCK:   st->blocked.sig[i] |= set->sig[i]; break;
            case SIG_UNBLOCK: st->blocked.sig[i] &= ~set->sig[i]; break;
            case SIG_SETMASK: st->blocked.sig[i] = set->sig[i]; break;
            default: return -1;
            }
        }
        /* SIGKILL/SIGSTOP are never blockable. */
        sigset_del(&st->blocked, SIGKILL);
        sigset_del(&st->blocked, SIGSTOP);
    }
    return 0;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    return rt_sigprocmask(how, set, oldset);
}

bool signal_has_pending(process_t* proc) {
    signal_state_t* st = proc ? (signal_state_t*)proc->signal_state : NULL;
    if (!st) {
        return false;
    }
    for (int sig = 1; sig < NSIG; sig++) {
        if (sigset_test(&st->pending, sig) && !sigset_test(&st->blocked, sig)) {
            return true;
        }
    }
    return false;
}

/* Frame builder + sigreturn live in signal_frame.c. */
extern int signal_build_user_frame(process_t* proc, signal_state_t* st,
                                   int sig, sigcontext_t* ctx);

int signal_dispatch(process_t* proc, sigcontext_t* ctx, bool on_syscall) {
    (void)on_syscall;
    if (!proc) {
        return 0;
    }
    signal_state_t* st = (signal_state_t*)proc->signal_state;
    if (!st) {
        return 0;
    }

    for (int sig = 1; sig < NSIG; sig++) {
        if (!sigset_test(&st->pending, sig)) {
            continue;
        }
        if (sigset_test(&st->blocked, sig) && sig != SIGKILL && sig != SIGSTOP) {
            continue;   /* masked: leave pending */
        }
        sigset_del(&st->pending, sig);

        sighandler_t h = st->actions[sig].handler;

        if (h == SIG_IGN) {
            continue;
        }
        if (h == SIG_DFL) {
            if (sig_default_terminates(sig)) {
                /* Default action: terminate. Only the running task can be torn
                 * down here (it is the one about to return to user mode). */
                kthread_exit(128 + sig);   /* never returns */
            }
            continue;   /* default ignore (SIGCHLD/SIGCONT) */
        }

        /* Custom handler: set up a user-stack signal frame and enter it. */
        if (signal_build_user_frame(proc, st, sig, ctx) == 0) {
            return sig;
        }
        /* Frame build failed (bad stack): fall back to killing the task. */
        kthread_exit(128 + sig);
    }
    return 0;
}
