bits 64
section .text

global syscall_entry
syscall_entry:
    ; Save user context
    swapgs
    mov [gs:0x10], rsp  ; Save user stack
    mov rsp, [gs:0x08]  ; Load kernel stack
    
    ; Save registers
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
    
    ; Call syscall handler
    ; syscall puts: rax = syscall number, rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5, r9 = arg6
    ; We need to rearrange for C calling convention
    mov r11, rax  ; Save syscall number
    mov rax, rdi  ; arg1 -> rax (temp)
    mov rdi, r11  ; syscall number -> rdi (first param)
    mov r11, rsi  ; Save arg2
    mov rsi, rax  ; arg1 -> rsi (second param)
    mov rax, rdx  ; Save arg3
    mov rdx, r11  ; arg2 -> rdx (third param)
    mov r11, r10  ; Save arg4
    mov rcx, rax  ; arg3 -> rcx (fourth param, but we use r10)
    mov r10, r11  ; arg4 -> r10 (fifth param)
    ; r8 already has arg5 (sixth param)
    
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
    
    ; Restore user context
    mov rsp, [gs:0x10]  ; Restore user stack
    swapgs
    
    sysretq
