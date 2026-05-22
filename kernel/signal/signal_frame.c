#include "signal.h"
#include "process.h"
#include "types.h"
#include "memory.h"
#include "exec.h"
#include "stdio.h"
#include "string.h"

typedef struct sigframe {
    u64 restorer;
    u64 signo;
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    u64 rip;
    u64 rflags;
} sigframe_t;

#define USER_SIGFRAME  (USER_STACK_TOP - 512)

int signal_deliver_pending(process_t* proc) {
    if (!proc) {
        return -1;
    }
    typedef struct {
        sigaction_t actions[NSIG];
        sigset_t pending;
        sigset_t blocked;
        sigset_t ignored;
    } signal_info_t;

    signal_info_t* si = (signal_info_t*)proc->private_data;
    if (!si) {
        return -1;
    }

    for (int sig = 1; sig < NSIG; sig++) {
        if (!(si->pending.sig[sig / 64] & (1ULL << (sig % 64)))) {
            continue;
        }
        si->pending.sig[sig / 64] &= ~(1ULL << (sig % 64));

        sighandler_t handler = si->actions[sig].handler;
        if (handler == SIG_IGN || handler == SIG_DFL) {
            continue;
        }

        sigframe_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.signo = (u64)sig;
        frame.restorer = 0;
        frame.rip = proc->rip;
        frame.rsp = proc->rsp;
        frame.rflags = proc->rflags;

        proc->rip = (u64)handler;
        proc->rsp = USER_SIGFRAME - sizeof(sigframe_t);
        memcpy((void*)proc->rsp, &frame, sizeof(frame));
        return sig;
    }
    return 0;
}

void signal_return_from_handler(void) {
    process_t* proc = process_current();
    if (!proc) {
        return;
    }
    sigframe_t* frame = (sigframe_t*)(uintptr_t)(proc->rsp);
    proc->rip = frame->rip;
    proc->rsp = frame->rsp;
    proc->rflags = frame->rflags;
}
