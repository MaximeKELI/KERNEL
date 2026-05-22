bits 64
section .text

; void exec_iretq_user(u64 rip, u64 rsp, u64 rflags)
global exec_iretq_user
exec_iretq_user:
    mov rax, 0x23          ; user CS
    mov rdx, 0x2b          ; user SS (CS+8 for SYSRET layout)
    push rdx               ; SS
    push rsi               ; RSP
    push rcx               ; RFLAGS
    push rax               ; CS
    push rdi               ; RIP
    iretq
