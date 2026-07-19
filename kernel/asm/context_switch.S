; Kernel-thread context switch (System V AMD64).
;
; Only the callee-saved registers (rbx, rbp, r12-r15) need to be preserved
; across switch_to(): per the ABI the caller has already spilled everything
; else. The FPU/SSE state of an *asynchronously preempted* task is preserved by
; isr_common (kernel/interrupt/idt_asm.S), which fxsave/fxrstor around the whole
; interrupt, so it does not need to be handled here.

bits 64
section .text

; void switch_to(u64* save_rsp /* rdi */, u64* load_rsp /* rsi */)
;   save_rsp -> &prev->rsp   (outgoing task)
;   load_rsp -> &next->rsp   (incoming task)
; Saves the outgoing callee-saved regs on its stack, records rsp, loads the
; incoming rsp and unwinds into the incoming task (which is either resuming
; inside switch_to too, or a brand-new task starting at kthread_trampoline).
global switch_to
switch_to:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp          ; prev->rsp = current kernel stack pointer
    mov rsp, [rsi]          ; load next->rsp

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; Entry point of every freshly-created kernel thread. The initial stack built in
; process_create() leaves the thread entry in r15 and its argument in r14 (both
; callee-saved, so they survive the schedule_tail() C call).
extern schedule_tail
extern thread_exit
global kthread_trampoline
kthread_trampoline:
    call schedule_tail      ; finish_task_switch: release runqueue + enable IRQs
    mov rdi, r14            ; arg
    call r15                ; entry(arg)
    call thread_exit        ; entry returned -> terminate the thread (never returns)
.hang:
    hlt
    jmp .hang

; int reg_integrity_loop(u64 sig /* rdi */, volatile int* stop /* rsi */)
; Test helper: keeps `sig` live in every callee-saved register across a loop in
; which the timer preempts us. Returns 1 if any register got clobbered by a
; context switch (i.e. switch_to failed to preserve callee-saved state), else 0.
global reg_integrity_loop
reg_integrity_loop:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rbx, rdi
    mov r12, rdi
    mov r13, rdi
    mov r14, rdi
    mov r15, rdi
.loop:
    cmp rbx, rdi
    jne .bad
    cmp r12, rdi
    jne .bad
    cmp r13, rdi
    jne .bad
    cmp r14, rdi
    jne .bad
    cmp r15, rdi
    jne .bad

    mov rcx, 20000          ; spin so the timer has a chance to preempt us here
.delay:
    pause
    dec rcx
    jnz .delay

    mov eax, [rsi]          ; stop flag?
    test eax, eax
    jz .loop

    xor eax, eax            ; stop requested -> success
    jmp .done
.bad:
    mov eax, 1
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
