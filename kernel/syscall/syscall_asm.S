bits 64
section .text

; SYSCALL entry (LSTAR target).
;
; On entry (set by the CPU):
;   rcx = user RIP to return to (must be preserved for SYSRET)
;   r11 = user RFLAGS to restore (must be preserved for SYSRET)
;   rax = syscall number
;   rdi, rsi, rdx, r10, r8, r9 = args 1..6  (r10 replaces rcx vs. the C ABI)
;   IF is masked by SFMASK, so we run with interrupts disabled and cannot be
;   preempted or reentered here.
;
; SYSCALL does NOT switch stacks, so we manually move to the current task's
; kernel stack. Everything the user must get back (all GP regs except rax/rcx/r11,
; plus the user rip/rflags/rsp) is saved on that kernel stack, making the path
; safe even if the C handler blocks and the task is rescheduled.

extern syscall_handler
extern syscall_kernel_stack
extern saved_user_rsp_tmp

global syscall_entry
syscall_entry:
    ; Switch to the kernel stack (stash the user rsp through a scratch cell:
    ; safe because SYSCALL runs with IF=0 and is not reentrant on this CPU).
    mov [saved_user_rsp_tmp], rsp
    mov rsp, [syscall_kernel_stack]

    ; Save everything needed to resume the user thread exactly.
    push qword [saved_user_rsp_tmp]   ; user rsp
    push r11                          ; user rflags (for sysretq)
    push rcx                          ; user rip    (for sysretq)
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9

    ; Marshal into the C calling convention:
    ;   syscall_handler(num=rdi, a1=rsi, a2=rdx, a3=rcx, a4=r8, a5=r9)
    ; The source registers are still live (push does not modify them).
    mov r9, r8      ; a5
    mov r8, r10     ; a4
    mov rcx, rdx    ; a3
    mov rdx, rsi    ; a2
    mov rsi, rdi    ; a1
    mov rdi, rax    ; syscall number

    ; 15 pushes above leave rsp at (16-aligned top - 120), i.e. rsp%16 == 8.
    ; The SysV ABI requires rsp%16 == 0 at the call site, else 16-byte-aligned
    ; stack spills (movaps) inside the C handler #GP.
    ;
    ; `push rsp` here does double duty: it re-aligns the stack (one push =>
    ; rsp%16 == 0) AND passes the address of the saved-register block (the value
    ; of rsp before the push, which points at the pushes above) as the 7th
    ; argument to syscall_handler (arg7 lives at [rsp] at the call site). fork()
    ; reads it to clone the caller's exact user context.
    push rsp
    call syscall_handler
    add rsp, 8
    ; rax = return value (kept across the restores below)

    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rcx                           ; user rip   -> rcx for sysretq
    pop r11                           ; user rflags-> r11 for sysretq
    pop rsp                           ; back to the user stack

    ; NASM has no "sysretq" mnemonic — that assembles as a *label*, silently
    ; dropping the instruction and falling through into the next function in
    ; kernel mode. Force the 64-bit form with the o64 (REX.W) prefix so we
    ; return to CPL=3 at rcx with rflags from r11.
    o64 sysret
