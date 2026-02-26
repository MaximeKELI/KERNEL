; Optimized context switch
; Complete register save/restore including FPU/SSE

bits 64
section .text

; Full context switch
; void context_switch_asm(void* from, void* to)
; Structure layout:
;   offset 0:   r15
;   offset 8:   r14
;   offset 16:  r13
;   offset 24:  r12
;   offset 32:  r11
;   offset 40:  r10
;   offset 48:  r9
;   offset 50:  r8
;   offset 56:  rdi
;   offset 64:  rsi
;   offset 72:  rbp
;   offset 80:  rbx
;   offset 88:  rdx
;   offset 96:  rcx
;   offset 104: rax
;   offset 112: rsp
;   offset 120: rip
;   offset 128: rflags
;   offset 136: cr3
;   offset 144: FPU state (512 bytes)
;   offset 656: SSE state (256 bytes)

global context_switch_asm
context_switch_asm:
    ; Save current context
    test rdi, rdi
    jz .skip_save
    
    ; Save general registers
    mov [rdi + 0], r15
    mov [rdi + 8], r14
    mov [rdi + 16], r13
    mov [rdi + 24], r12
    mov [rdi + 32], r11
    mov [rdi + 40], r10
    mov [rdi + 48], r9
    mov [rdi + 56], r8
    mov [rdi + 64], rdi
    mov [rdi + 72], rsi
    mov [rdi + 80], rbp
    mov [rdi + 88], rbx
    mov [rdi + 96], rdx
    mov [rdi + 104], rcx
    mov [rdi + 112], rax
    
    ; Save stack pointer
    mov [rdi + 120], rsp
    
    ; Save instruction pointer (return address)
    mov rax, [rsp]
    mov [rdi + 128], rax
    
    ; Save flags
    pushfq
    pop rax
    mov [rdi + 136], rax
    
    ; Save CR3
    mov rax, cr3
    mov [rdi + 144], rax
    
    ; Save FPU state
    lea rax, [rdi + 152]
    fxsave64 [rax]
    
    ; Save SSE state (XMM registers)
    lea rax, [rdi + 664]
    movdqu [rax], xmm0
    movdqu [rax + 16], xmm1
    movdqu [rax + 32], xmm2
    movdqu [rax + 48], xmm3
    movdqu [rax + 64], xmm4
    movdqu [rax + 80], xmm5
    movdqu [rax + 96], xmm6
    movdqu [rax + 112], xmm7
    movdqu [rax + 128], xmm8
    movdqu [rax + 144], xmm9
    movdqu [rax + 160], xmm10
    movdqu [rax + 176], xmm11
    movdqu [rax + 192], xmm12
    movdqu [rax + 208], xmm13
    movdqu [rax + 224], xmm14
    movdqu [rax + 240], xmm15
    
.skip_save:
    ; Restore new context
    mov rdi, rsi       ; to -> rdi
    
    ; Restore CR3 (page tables)
    mov rax, [rdi + 144]
    mov cr3, rax
    
    ; Restore FPU state
    lea rax, [rdi + 152]
    fxrstor64 [rax]
    
    ; Restore SSE state
    lea rax, [rdi + 664]
    movdqu xmm0, [rax]
    movdqu xmm1, [rax + 16]
    movdqu xmm2, [rax + 32]
    movdqu xmm3, [rax + 48]
    movdqu xmm4, [rax + 64]
    movdqu xmm5, [rax + 80]
    movdqu xmm6, [rax + 96]
    movdqu xmm7, [rax + 112]
    movdqu xmm8, [rax + 128]
    movdqu xmm9, [rax + 144]
    movdqu xmm10, [rax + 160]
    movdqu xmm11, [rax + 176]
    movdqu xmm12, [rax + 192]
    movdqu xmm13, [rax + 208]
    movdqu xmm14, [rax + 224]
    movdqu xmm15, [rax + 240]
    
    ; Restore general registers
    mov r15, [rdi + 0]
    mov r14, [rdi + 8]
    mov r13, [rdi + 16]
    mov r12, [rdi + 24]
    mov r11, [rdi + 32]
    mov r10, [rdi + 40]
    mov r9, [rdi + 48]
    mov r8, [rdi + 56]
    mov rsi, [rdi + 64]
    mov rbp, [rdi + 72]
    mov rbx, [rdi + 80]
    mov rdx, [rdi + 88]
    mov rcx, [rdi + 96]
    mov rax, [rdi + 104]
    
    ; Restore stack pointer
    mov rsp, [rdi + 120]
    
    ; Restore flags
    mov rax, [rdi + 136]
    push rax
    popfq
    
    ; Restore instruction pointer (jump to saved RIP)
    mov rax, [rdi + 128]
    jmp rax
