; Cache control operations
; Critical for memory consistency and performance

bits 64
section .text

; Flush cache line
; void clflush_asm(void* addr)
global clflush_asm
clflush_asm:
    clflush [rdi]
    mfence              ; Memory fence
    ret

; Flush cache line and invalidate
; void clflushopt_asm(void* addr)
global clflushopt_asm
clflushopt_asm:
    clflushopt [rdi]    ; Requires CLFLUSHOPT support
    sfence              ; Store fence
    ret

; Prefetch data into cache
; void prefetch_asm(void* addr, u32 hint)
; hint: 0=T0 (temporal), 1=T1, 2=T2, 3=NTA (non-temporal)
global prefetch_asm
prefetch_asm:
    test esi, esi
    jz .t0
    cmp esi, 1
    je .t1
    cmp esi, 2
    je .t2
    ; NTA
    prefetchnta [rdi]
    ret
.t0:
    prefetcht0 [rdi]
    ret
.t1:
    prefetcht1 [rdi]
    ret
.t2:
    prefetcht2 [rdi]
    ret

; Memory fence (full barrier)
; void mfence_asm(void)
global mfence_asm
mfence_asm:
    mfence
    ret

; Load fence (read barrier)
; void lfence_asm(void)
global lfence_asm
lfence_asm:
    lfence
    ret

; Store fence (write barrier)
; void sfence_asm(void)
global sfence_asm
sfence_asm:
    sfence
    ret

; Write back and invalidate cache
; void wbinvd_asm(void)
global wbinvd_asm
wbinvd_asm:
    wbinvd              ; Requires privilege
    ret

; Invalidate cache
; void invd_asm(void)
global invd_asm
invd_asm:
    invd                ; Requires privilege
    ret
