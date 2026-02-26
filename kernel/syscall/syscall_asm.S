bits 64
section .text

global syscall_entry
syscall_entry:
    ; Save user context (simplified - no swapgs for now)
    ; Save all registers
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    
    ; syscall instruction puts:
    ;   rax = syscall number
    ;   rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5
    ; Our C function expects: syscall_handler(syscall_num, arg1, arg2, arg3, arg4, arg5)
    ; x86_64 calling convention: rdi, rsi, rdx, rcx, r8, r9
    
    ; Get values from stack (they were pushed in reverse order)
    ; Stack: rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15
    ; But we need original values before push
    ; rax on stack = syscall number
    ; rdi on stack = arg1 (original)
    ; rsi on stack = arg2 (original)
    ; rdx on stack = arg3 (original)
    ; r10 on stack = arg4 (original)
    ; r8 on stack = arg5 (original)
    
    ; Rearrange for C calling convention
    mov rdi, [rsp + 14*8]  ; syscall number (rax was last pushed)
    mov rsi, [rsp + 9*8]   ; arg1 (rdi)
    mov rdx, [rsp + 8*8]   ; arg2 (rsi)
    mov rcx, [rsp + 7*8]   ; arg3 (rdx)
    mov r8, [rsp + 5*8]    ; arg4 (r10)
    mov r9, [rsp + 6*8]    ; arg5 (r8)
    
    extern syscall_handler
    call syscall_handler
    
    ; Restore registers
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    
    sysretq
