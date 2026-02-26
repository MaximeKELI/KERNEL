; SIMD vector operations
; Optimized vector math and data processing

bits 64
section .text

; Vector add (16 bytes at a time)
; void simd_add_16(void* dst, const void* src1, const void* src2)
global simd_add_16
simd_add_16:
    movdqu xmm0, [rsi]  ; Load src1
    movdqu xmm1, [rdx]  ; Load src2
    paddb xmm0, xmm1    ; Add bytes
    movdqu [rdi], xmm0  ; Store result
    ret

; Vector multiply (16 bytes at a time)
; void simd_mul_16(void* dst, const void* src1, const void* src2)
global simd_mul_16
simd_mul_16:
    movdqu xmm0, [rsi]  ; Load src1
    movdqu xmm1, [rdx]  ; Load src2
    pmullw xmm0, xmm1   ; Multiply words
    movdqu [rdi], xmm0  ; Store result
    ret

; Vector compare (16 bytes)
; u32 simd_cmp_16(const void* src1, const void* src2)
global simd_cmp_16
simd_cmp_16:
    movdqu xmm0, [rdi]  ; Load src1
    movdqu xmm1, [rsi]  ; Load src2
    pcmpeqb xmm0, xmm1  ; Compare bytes
    pmovmskb eax, xmm0  ; Extract mask
    ret

; Vector zero (set 16 bytes to zero)
; void simd_zero_16(void* dst)
global simd_zero_16
simd_zero_16:
    pxor xmm0, xmm0     ; Zero xmm0
    movdqu [rdi], xmm0  ; Store zeros
    ret

; Vector copy (16 bytes)
; void simd_copy_16(void* dst, const void* src)
global simd_copy_16
simd_copy_16:
    movdqu xmm0, [rsi]  ; Load src
    movdqu [rdi], xmm0  ; Store dst
    ret

; Vector sum (sum 16 bytes)
; u32 simd_sum_16(const void* src)
global simd_sum_16
simd_sum_16:
    movdqu xmm0, [rdi]  ; Load src
    pxor xmm1, xmm1     ; Zero accumulator
    psadbw xmm1, xmm0   ; Sum of absolute differences
    movd eax, xmm1      ; Extract sum
    ret

; Vector find minimum (16 bytes)
; u8 simd_min_16(const void* src)
global simd_min_16
simd_min_16:
    movdqu xmm0, [rdi]  ; Load src
    movdqa xmm1, xmm0   ; Copy
    psrlw xmm1, 8       ; Shift to get high bytes
    pminub xmm0, xmm1   ; Minimum
    movd eax, xmm0      ; Extract
    and eax, 0xFF       ; Get byte
    ret

; Vector find maximum (16 bytes)
; u8 simd_max_16(const void* src)
global simd_max_16
simd_max_16:
    movdqu xmm0, [rdi]  ; Load src
    movdqa xmm1, xmm0   ; Copy
    psrlw xmm1, 8       ; Shift
    pmaxub xmm0, xmm1   ; Maximum
    movd eax, xmm0      ; Extract
    and eax, 0xFF       ; Get byte
    ret
