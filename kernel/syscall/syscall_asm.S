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
    ; Arguments are already in correct registers from syscall
    ; rax = syscall number, rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5
    mov rdi, rax  ; syscall number
    mov rsi, rdi  ; arg1 (save original)
    mov rdx, rsi  ; arg2 (save original)
    mov rcx, rdx  ; arg3 (save original)
    mov r8, r10   ; arg4 (save original)
    mov r9, r8    ; arg5 (save original)
    
    ; Now restore original arg positions
    mov rdi, rsi  ; arg1
    mov rsi, rdx  ; arg2
    mov rdx, rcx  ; arg3
    mov r10, r8   ; arg4
    mov r8, r9    ; arg5
    mov rax, rdi  ; syscall number
    
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
