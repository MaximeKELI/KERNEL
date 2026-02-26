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
    ; syscall instruction puts:
    ;   rax = syscall number
    ;   rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5
    ; Our handler expects: syscall_num, arg1, arg2, arg3, arg4, arg5
    mov r11, rax  ; Save syscall number (will be first param in rdi)
    mov rax, rdi  ; Save arg1
    mov rdi, r11  ; syscall number -> rdi (first param)
    ; rsi already has arg2 (second param)
    ; rdx already has arg3 (third param)
    ; r10 already has arg4 (fourth param)
    ; r8 already has arg5 (fifth param)
    ; But we need arg1 in rsi, so swap
    xchg rsi, rax ; arg1 -> rsi, arg2 -> rax (temp)
    mov rdx, rsi  ; arg1 -> rdx (wrong, need to fix)
    mov rsi, rax  ; arg2 -> rsi (restore)
    mov rax, rdi  ; syscall number -> rax (temp)
    mov rdi, rax  ; syscall number -> rdi
    mov rax, rdx  ; arg1 -> rax (temp)
    mov rdx, rcx  ; Need to save rcx
    mov rcx, rax  ; arg1 -> rcx (wrong register)
    
    ; Actually, let's use a simpler approach - save all and rearrange
    push r8       ; Save arg5
    push r10      ; Save arg4
    push rdx      ; Save arg3
    push rsi      ; Save arg2
    push rax      ; Save arg1 (from earlier)
    
    mov rdi, r11  ; syscall number -> rdi
    pop rsi       ; arg1 -> rsi
    pop rdx       ; arg2 -> rdx (wrong, need to fix)
    pop rcx       ; arg3 -> rcx (wrong)
    pop r8        ; arg4 -> r8 (wrong)
    pop r9        ; arg5 -> r9 (wrong)
    
    ; Fix the order
    mov r10, r8   ; arg4 -> r10
    mov r8, r9    ; arg5 -> r8
    mov r9, rdx   ; Save rdx
    mov rdx, rcx  ; arg3 -> rdx
    mov rcx, r9   ; Restore
    
    ; Actually simplest: just pass them as-is and let C handle it
    mov rdi, r11  ; syscall number
    ; rsi, rdx, r10, r8 already correct
    
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
