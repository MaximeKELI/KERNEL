#include "signal.h"
#include "process.h"
#include "stdio.h"
#include "debug.h"
#include "memory.h"
#include "interrupt.h"

/* Signal handlers per process */
typedef struct signal_info {
    sigaction_t actions[NSIG];
    sigset_t pending;
    sigset_t blocked;
    sigset_t ignored;
} signal_info_t;

static void default_signal_handler(int sig) {
    DEBUG_INFO("Default handler for signal %d", sig);
    if (sig == SIGKILL || sig == SIGSTOP) {
        process_t* proc = process_current();
        if (proc) {
            proc->state = PROCESS_DEAD;
        }
    }
}

void signal_init(void) {
    DEBUG_INFO("Signal system initialized");
}

int kill(u64 pid, int sig) {
    if (sig < 1 || sig >= NSIG) {
        return -1;
    }
    
    /* Find process */
    extern process_t* process_list;
    process_t* proc = process_list;
    while (proc) {
        if (proc->pid == pid) {
            /* Add signal to pending */
            signal_info_t* sig_info = (signal_info_t*)proc->files; /* Reuse files pointer */
            if (sig_info) {
                sig_info->pending.sig[sig / 64] |= (1ULL << (sig % 64));
            }
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

sighandler_t signal(int sig, sighandler_t handler) {
    if (sig < 1 || sig >= NSIG) {
        return SIG_ERR;
    }
    
    process_t* proc = process_current();
    if (!proc) return SIG_ERR;
    
    signal_info_t* sig_info = (signal_info_t*)proc->files;
    if (!sig_info) {
        sig_info = (signal_info_t*)kzalloc(sizeof(signal_info_t));
        if (!sig_info) return SIG_ERR;
        proc->files = sig_info;
    }
    
    sighandler_t old = sig_info->actions[sig].handler;
    sig_info->actions[sig].handler = handler;
    
    return old;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    process_t* proc = process_current();
    if (!proc) return -1;
    
    signal_info_t* sig_info = (signal_info_t*)proc->files;
    if (!sig_info) {
        sig_info = (signal_info_t*)kzalloc(sizeof(signal_info_t));
        if (!sig_info) return -1;
        proc->files = sig_info;
    }
    
    if (oldset) {
        *oldset = sig_info->blocked;
    }
    
    if (set) {
        switch (how) {
            case 0: /* SIG_BLOCK */
                for (int i = 0; i < NSIG / 64; i++) {
                    sig_info->blocked.sig[i] |= set->sig[i];
                }
                break;
            case 1: /* SIG_UNBLOCK */
                for (int i = 0; i < NSIG / 64; i++) {
                    sig_info->blocked.sig[i] &= ~set->sig[i];
                }
                break;
            case 2: /* SIG_SETMASK */
                sig_info->blocked = *set;
                break;
        }
    }
    
    return 0;
}

int sigwait(const sigset_t* set, int* sig) {
    process_t* proc = process_current();
    if (!proc) return -1;
    
    signal_info_t* sig_info = (signal_info_t*)proc->files;
    if (!sig_info) return -1;
    
    /* Wait for signal */
    while (true) {
        for (int i = 1; i < NSIG; i++) {
            if (sig_info->pending.sig[i / 64] & (1ULL << (i % 64))) {
                if (!set || (set->sig[i / 64] & (1ULL << (i % 64)))) {
                    sig_info->pending.sig[i / 64] &= ~(1ULL << (i % 64));
                    *sig = i;
                    return 0;
                }
            }
        }
        /* Sleep until signal */
        __asm__ __volatile__("hlt");
    }
}
