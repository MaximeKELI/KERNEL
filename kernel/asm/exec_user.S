bits 64
section .text

; void exec_iretq_user(u64 rip /* rdi */, u64 rsp /* rsi */, u64 rflags /* rdx */)
; Build an inter-privilege iretq frame and drop to ring 3 (CS=0x23, SS=0x2b).
global exec_iretq_user
exec_iretq_user:
    mov rax, 0x23          ; user CS  (RPL 3)
    mov rcx, 0x2b          ; user SS  (CS+8 for the SYSRET layout)
    push rcx               ; SS
    push rsi               ; RSP
    push rdx               ; RFLAGS   (3rd argument — must carry IF=1 for preemption)
    push rax               ; CS
    push rdi               ; RIP
    iretq
