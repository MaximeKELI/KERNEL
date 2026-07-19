#include "signal.h"
#include "process.h"
#include "trapframe.h"
#include "interrupt.h"
#include "syscall.h"
#include "types.h"
#include "memory.h"
#include "string.h"
#include "exec.h"
#include "stdio.h"

/*
 * Real ring-3 signal delivery.
 *
 * When a handler is invoked we push a frame onto the *user* stack containing the
 * saved register context plus a tiny trampoline, then rewrite the return-to-user
 * register image so control resumes at the handler with:
 *   rdi = signo, rsp -> the frame (so [rsp] = trampoline return address).
 * When the handler returns it "ret"s into the trampoline, which does
 *   rt_sigreturn(), restoring the saved context verbatim.
 *
 * NX is not enabled on this kernel, so executing the trampoline off the user
 * stack is legal (this is exactly how classic Linux delivered signals before
 * sa_restorer/vDSO).
 */

/* signal_state layout duplicated minimally: we only touch `blocked` here. */
typedef struct signal_state {
    sigaction_t actions[NSIG];
    sigset_t pending;
    sigset_t blocked;
} signal_state_t;

typedef struct rt_sigframe {
    u64 ret_addr;          /* handler returns here -> &tramp */
    sigcontext_t ctx;      /* saved user context */
    sigset_t saved_mask;   /* blocked mask to restore on sigreturn */
    u8 tramp[16];          /* mov eax, SYS_SIGRETURN ; syscall */
} rt_sigframe_t;

/* Build "mov eax, imm32 ; syscall" into buf (7 bytes). */
static void emit_sigreturn_tramp(u8* buf) {
    buf[0] = 0xB8;                         /* mov eax, imm32 */
    buf[1] = (u8)(SYS_SIGRETURN & 0xff);
    buf[2] = (u8)((SYS_SIGRETURN >> 8) & 0xff);
    buf[3] = (u8)((SYS_SIGRETURN >> 16) & 0xff);
    buf[4] = (u8)((SYS_SIGRETURN >> 24) & 0xff);
    buf[5] = 0x0F;                         /* syscall */
    buf[6] = 0x05;
}

int signal_build_user_frame(process_t* proc, signal_state_t* st,
                            int sig, sigcontext_t* ctx) {
    (void)proc;

    /* Reserve the frame on the user stack, keeping it 16-byte aligned so that
     * after the handler's implicit "call" (rsp points at ret_addr) the ABI's
     * rsp%16==8-at-entry contract holds. */
    u64 sp = ctx->rsp;
    sp -= sizeof(rt_sigframe_t);
    sp &= ~0xFULL;
    sp -= 8;                 /* handler entry wants rsp%16 == 8 (post-"call") */

    rt_sigframe_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.ctx = *ctx;                       /* full context to resume later */
    frame.saved_mask = st->blocked;
    emit_sigreturn_tramp(frame.tramp);
    frame.ret_addr = sp + __builtin_offsetof(rt_sigframe_t, tramp);

    /* Copy the frame into user memory (active address space is the target's). */
    memcpy((void*)sp, &frame, sizeof(frame));

    /* Block this signal (plus the handler's sa_mask) for the handler's run. */
    st->blocked.sig[sig / 64] |= (1ULL << (sig % 64));
    for (int i = 0; i < (NSIG + 63) / 64; i++) {
        st->blocked.sig[i] |= st->actions[sig].mask.sig[i];
    }

    /* Enter the handler: SysV first arg (signo) in rdi, stack at the frame. */
    ctx->rip = (u64)st->actions[sig].handler;
    ctx->rsp = sp;
    ctx->rdi = (u64)sig;
    return 0;
}

/*
 * rt_sigreturn: the trampoline invoked this syscall. At this point the user rsp
 * points just past ret_addr, i.e. at rt_sigframe.ctx. Restore that context into
 * the live syscall frame so the SYSRET returns to the interrupted instruction.
 * Returns the saved rax (restored as the syscall's return value).
 */
u64 sys_rt_sigreturn(void) {
    process_t* proc = process_current();
    if (!proc || !proc->syscall_regs) {
        return (u64)-1;
    }
    syscall_frame_t* sf = (syscall_frame_t*)proc->syscall_regs;

    /* user rsp currently points at rt_sigframe.ctx (handler popped ret_addr). */
    u64 ctx_addr = sf->user_rsp;
    sigcontext_t ctx;
    memcpy(&ctx, (void*)ctx_addr, sizeof(ctx));

    sigset_t saved_mask;
    memcpy(&saved_mask, (void*)(ctx_addr + sizeof(sigcontext_t)), sizeof(saved_mask));

    /* Restore the blocked mask that was in effect before the handler. */
    signal_state_t* st = (signal_state_t*)proc->signal_state;
    if (st) {
        st->blocked = saved_mask;
    }

    /* Rewrite the live syscall frame from the saved context. */
    sf->user_rip = ctx.rip;
    sf->user_rsp = ctx.rsp;
    sf->user_rflags = ctx.rflags | 0x202;
    sf->rbx = ctx.rbx;
    sf->rbp = ctx.rbp;
    sf->rdi = ctx.rdi;
    sf->rsi = ctx.rsi;
    sf->rdx = ctx.rdx;
    sf->r8 = ctx.r8;
    sf->r9 = ctx.r9;
    sf->r10 = ctx.r10;
    sf->r12 = ctx.r12;
    sf->r13 = ctx.r13;
    sf->r14 = ctx.r14;
    sf->r15 = ctx.r15;

    return ctx.rax;   /* becomes rax after the sysret */
}

void signal_return_from_handler(void) {
    (void)sys_rt_sigreturn();
}

/*
 * Deliver a pending signal on the way out of an interrupt that returns to ring
 * 3 (timer preemption, device IRQ). Lets asynchronous signals (e.g. kill from
 * another process) reach a task that is busy-looping in userspace, not just one
 * sitting in a syscall. Rewrites the interrupt frame to enter the handler.
 */
void signal_check_on_irq_return(interrupt_frame_t* frame) {
    process_t* proc = process_current();
    if (!proc || !signal_has_pending(proc)) {
        return;
    }
    sigcontext_t ctx;
    ctx.rax = frame->rax; ctx.rbx = frame->rbx; ctx.rcx = frame->rcx;
    ctx.rdx = frame->rdx; ctx.rsi = frame->rsi; ctx.rdi = frame->rdi;
    ctx.rbp = frame->rbp; ctx.r8 = frame->r8; ctx.r9 = frame->r9;
    ctx.r10 = frame->r10; ctx.r11 = frame->r11; ctx.r12 = frame->r12;
    ctx.r13 = frame->r13; ctx.r14 = frame->r14; ctx.r15 = frame->r15;
    ctx.rip = frame->rip; ctx.rsp = frame->rsp; ctx.rflags = frame->rflags;

    if (signal_dispatch(proc, &ctx, false) > 0) {
        frame->rip = ctx.rip;
        frame->rsp = ctx.rsp;
        frame->rflags = ctx.rflags;
        frame->rdi = ctx.rdi;   /* signo */
    }
}
