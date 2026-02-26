; Optimized string operations
; High-performance memcpy, memset, memcmp, strlen, etc.

bits 64
section .text

; Optimized memcpy
; void* memcpy_asm(void* dest, const void* src, size_t n)
global memcpy_asm
memcpy_asm:
    mov rax, rdi       ; Return destination
    mov rcx, rdx       ; Count
    
    ; Handle zero length
    test rcx, rcx
    jz .done
    
    ; Use REP MOVSB for small copies
    cmp rcx, 64
    jb .small_copy
    
    ; Align destination to 16 bytes
    mov r8, rdi
    and r8, 15
    jz .aligned
    
    ; Copy unaligned bytes
    mov r9, 16
    sub r9, r8
    cmp rcx, r9
    cmovb r9, rcx
    sub rcx, r9
    rep movsb
    test rcx, rcx
    jz .done
    
.aligned:
    ; Copy 16 bytes at a time
    mov r8, rcx
    shr rcx, 4
    rep movsq
    mov rcx, r8
    and rcx, 15
    rep movsb
    jmp .done
    
.small_copy:
    rep movsb
    
.done:
    ret

; Optimized memset
; void* memset_asm(void* s, int c, size_t n)
global memset_asm
memset_asm:
    mov rax, rdi       ; Return destination
    mov rcx, rdx       ; Count
    
    ; Handle zero length
    test rcx, rcx
    jz .done
    
    ; Expand byte to 64-bit
    movzx r8, sil      ; c
    mov r9, r8
    shl r9, 8
    or r8, r9
    mov r9, r8
    shl r9, 16
    or r8, r9
    mov r9, r8
    shl r9, 32
    or r8, r9          ; r8 = c repeated 8 times
    
    ; Use REP STOSB for small sets
    cmp rdx, 64
    jb .small_set
    
    ; Set 8 bytes at a time
    mov rcx, rdx
    shr rcx, 3
    mov rax, r8
    rep stosq
    mov rcx, rdx
    and rcx, 7
    mov rax, r8
    rep stosb
    jmp .done
    
.small_set:
    mov rax, r8
    rep stosb
    
.done:
    mov rax, rdi
    ret

; Optimized memcmp
; int memcmp_asm(const void* s1, const void* s2, size_t n)
global memcmp_asm
memcmp_asm:
    mov rcx, rdx       ; Count
    
    ; Handle zero length
    test rcx, rcx
    jz .equal
    
    ; Compare 8 bytes at a time
    cmp rcx, 8
    jb .byte_compare
    
    mov r8, rcx
    shr rcx, 3
    repe cmpsq
    jnz .diff
    mov rcx, r8
    and rcx, 7
    jz .equal
    
.byte_compare:
    repe cmpsb
    jnz .diff
    
.equal:
    xor eax, eax
    ret
    
.diff:
    movzx eax, byte [rdi - 1]
    movzx ecx, byte [rsi - 1]
    sub eax, ecx
    ret

; Optimized strlen
; size_t strlen_asm(const char* s)
global strlen_asm
strlen_asm:
    mov rax, rdi       ; Start address
    mov rcx, -1        ; Maximum count
    
    ; Search for null terminator
    repne scasb
    
    ; Calculate length
    mov rax, rdi
    sub rax, rdi       ; rax = rdi - original rdi
    dec rax            ; Exclude null terminator
    ret

; Optimized strcmp
; int strcmp_asm(const char* s1, const char* s2)
global strcmp_asm
strcmp_asm:
    ; Compare byte by byte
.loop:
    mov al, [rdi]
    mov cl, [rsi]
    cmp al, cl
    jne .diff
    test al, al
    jz .equal
    inc rdi
    inc rsi
    jmp .loop
    
.equal:
    xor eax, eax
    ret
    
.diff:
    movzx eax, al
    movzx ecx, cl
    sub eax, ecx
    ret
