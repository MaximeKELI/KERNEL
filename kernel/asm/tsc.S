; Time Stamp Counter (TSC) operations
; Critical for high-precision timing

bits 64
section .text

; Read TSC
; u64 rdtsc_asm(void)
global rdtsc_asm
rdtsc_asm:
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret

; Read TSC with serialization
; u64 rdtscp_asm(u32* aux)
global rdtscp_asm
rdtscp_asm:
    rdtscp              ; Read TSC and processor ID
    shl rdx, 32
    or rax, rdx
    mov [rdi], ecx      ; Store processor ID in aux
    ret

; Read TSC and pause (for spin loops)
; u64 rdtsc_pause_asm(void)
global rdtsc_pause_asm
rdtsc_pause_asm:
    pause               ; Optimize spin loop
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret

; Calibrate TSC frequency
; u64 tsc_calibrate_asm(void)
global tsc_calibrate_asm
tsc_calibrate_asm:
    ; Would use PIT or HPET to calibrate
    ; For now, return TSC value
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret
