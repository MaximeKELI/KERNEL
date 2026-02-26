; FPU/SSE/AVX context save and restore
; Critical for proper context switching with floating point

bits 64
section .text

; Save FPU state
; void fpu_save(void* fpu_state)
global fpu_save
fpu_save:
    fxsave64 [rdi]
    ret

; Restore FPU state
; void fpu_restore(void* fpu_state)
global fpu_restore
fpu_restore:
    fxrstor64 [rdi]
    ret

; Initialize FPU
; void fpu_init(void)
global fpu_init
fpu_init:
    fninit
    ret

; Save SSE state (XMM registers)
; void sse_save(void* sse_state)
global sse_save
sse_save:
    movdqu [rdi], xmm0
    movdqu [rdi + 16], xmm1
    movdqu [rdi + 32], xmm2
    movdqu [rdi + 48], xmm3
    movdqu [rdi + 64], xmm4
    movdqu [rdi + 80], xmm5
    movdqu [rdi + 96], xmm6
    movdqu [rdi + 112], xmm7
    movdqu [rdi + 128], xmm8
    movdqu [rdi + 144], xmm9
    movdqu [rdi + 160], xmm10
    movdqu [rdi + 176], xmm11
    movdqu [rdi + 192], xmm12
    movdqu [rdi + 208], xmm13
    movdqu [rdi + 224], xmm14
    movdqu [rdi + 240], xmm15
    ret

; Restore SSE state (XMM registers)
; void sse_restore(void* sse_state)
global sse_restore
sse_restore:
    movdqu xmm0, [rdi]
    movdqu xmm1, [rdi + 16]
    movdqu xmm2, [rdi + 32]
    movdqu xmm3, [rdi + 48]
    movdqu xmm4, [rdi + 64]
    movdqu xmm5, [rdi + 80]
    movdqu xmm6, [rdi + 96]
    movdqu xmm7, [rdi + 112]
    movdqu xmm8, [rdi + 128]
    movdqu xmm9, [rdi + 144]
    movdqu xmm10, [rdi + 160]
    movdqu xmm11, [rdi + 176]
    movdqu xmm12, [rdi + 192]
    movdqu xmm13, [rdi + 208]
    movdqu xmm14, [rdi + 224]
    movdqu xmm15, [rdi + 240]
    ret

; Save AVX state (YMM registers) - requires AVX support
; void avx_save(void* avx_state)
global avx_save
avx_save:
    vmovdqu [rdi], ymm0
    vmovdqu [rdi + 32], ymm1
    vmovdqu [rdi + 64], ymm2
    vmovdqu [rdi + 96], ymm3
    vmovdqu [rdi + 128], ymm4
    vmovdqu [rdi + 160], ymm5
    vmovdqu [rdi + 192], ymm6
    vmovdqu [rdi + 224], ymm7
    vmovdqu [rdi + 256], ymm8
    vmovdqu [rdi + 288], ymm9
    vmovdqu [rdi + 320], ymm10
    vmovdqu [rdi + 352], ymm11
    vmovdqu [rdi + 384], ymm12
    vmovdqu [rdi + 416], ymm13
    vmovdqu [rdi + 448], ymm14
    vmovdqu [rdi + 480], ymm15
    ret

; Restore AVX state (YMM registers)
; void avx_restore(void* avx_state)
global avx_restore
avx_restore:
    vmovdqu ymm0, [rdi]
    vmovdqu ymm1, [rdi + 32]
    vmovdqu ymm2, [rdi + 64]
    vmovdqu ymm3, [rdi + 96]
    vmovdqu ymm4, [rdi + 128]
    vmovdqu ymm5, [rdi + 160]
    vmovdqu ymm6, [rdi + 192]
    vmovdqu ymm7, [rdi + 224]
    vmovdqu ymm8, [rdi + 256]
    vmovdqu ymm9, [rdi + 288]
    vmovdqu ymm10, [rdi + 320]
    vmovdqu ymm11, [rdi + 352]
    vmovdqu ymm12, [rdi + 384]
    vmovdqu ymm13, [rdi + 416]
    vmovdqu ymm14, [rdi + 448]
    vmovdqu ymm15, [rdi + 480]
    ret
