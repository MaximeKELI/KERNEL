bits 64
section .text

global syscall_entry
syscall_entry:
    ; syscall instruction provides:
    ;   rax = syscall number
    ;   rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5, r9 = arg6
    ; 
    ; We need to save registers BEFORE pushing, then rearrange for C calling convention
    ; C calling convention: rdi, rsi, rdx, rcx, r8, r9
    
    ; Save syscall number and arguments in callee-saved registers
    mov r11, rax  ; Save syscall number
    mov r12, rdi  ; Save arg1
    mov r13, rsi  ; Save arg2
    mov r14, rdx  ; Save arg3
    mov r15, r10  ; Save arg4
    ; r8 already has arg5, we'll handle it
    
    ; Now save all registers
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
    
    ; Rearrange for C calling convention using saved values
    mov rdi, r11  ; syscall number -> rdi (first param)
    mov rsi, r12  ; arg1 -> rsi (second param)
    mov rdx, r13  ; arg2 -> rdx (third param)
    mov rcx, r14  ; arg3 -> rcx (fourth param, but we'll use r10)
    mov r10, r15  ; arg4 -> r10 (fifth param)
    ; r8 already has arg5 (sixth param)
    
    extern syscall_handler
    call syscall_handler
    
    ; Save return value (in rax) before restoring
    mov r11, rax
    
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
    
    ; Restore return value
    mov rax, r11
    
    sysretq
