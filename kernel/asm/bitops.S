; Bit manipulation operations
; Critical for bitmaps and data structures

bits 64
section .text

; Find first set bit (LSB)
; u32 ffs_asm(u64 value)
global ffs_asm
ffs_asm:
    bsf rax, rdi        ; Bit scan forward
    jz .zero
    inc eax             ; BSF returns 0-based, we want 1-based
    ret
.zero:
    xor eax, eax
    ret

; Find last set bit (MSB)
; u32 fls_asm(u64 value)
global fls_asm
fls_asm:
    bsr rax, rdi        ; Bit scan reverse
    jz .zero
    inc eax             ; BSR returns 0-based, we want 1-based
    ret
.zero:
    xor eax, eax
    ret

; Count set bits (population count)
; u32 popcount_asm(u64 value)
global popcount_asm
popcount_asm:
    popcnt rax, rdi     ; Population count (requires POPCNT support)
    ret

; Count leading zeros
; u32 clz_asm(u64 value)
global clz_asm
clz_asm:
    bsr rax, rdi
    jz .all_zeros
    mov ecx, 63
    sub ecx, eax
    mov eax, ecx
    ret
.all_zeros:
    mov eax, 64
    ret

; Count trailing zeros
; u32 ctz_asm(u64 value)
global ctz_asm
ctz_asm:
    bsf rax, rdi
    jz .all_zeros
    ret
.all_zeros:
    mov eax, 64
    ret

; Rotate left
; u64 rol_asm(u64 value, u32 shift)
global rol_asm
rol_asm:
    mov cl, sil
    rol rdi, cl
    mov rax, rdi
    ret

; Rotate right
; u64 ror_asm(u64 value, u32 shift)
global ror_asm
ror_asm:
    mov cl, sil
    ror rdi, cl
    mov rax, rdi
    ret

; Bit reverse
; u64 bit_reverse_asm(u64 value)
global bit_reverse_asm
bit_reverse_asm:
    mov rax, rdi
    mov rcx, 64
.loop:
    shl rax, 1
    rcr rdx, 1
    dec rcx
    jnz .loop
    mov rax, rdx
    ret
