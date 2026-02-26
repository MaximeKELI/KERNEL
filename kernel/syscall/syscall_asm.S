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
    mov rdi, rax  ; syscall number
    mov rsi, rbx  ; arg1
    mov rdx, rcx  ; arg2
    mov r10, rdx  ; arg3
    mov r8, rsi   ; arg4
    mov r9, rdi   ; arg5
    
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
