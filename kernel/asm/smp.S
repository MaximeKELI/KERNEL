; SMP operations - Inter-Processor Interrupts (IPI)
; Critical for multi-core synchronization

bits 64
section .text

; Send IPI to specific CPU
; void send_ipi(u32 cpu_id, u32 vector)
global send_ipi
send_ipi:
    ; Calculate APIC ID
    mov eax, edi        ; cpu_id
    shl eax, 24         ; Shift to destination field
    
    ; Set delivery mode (fixed)
    or eax, 0x00004000  ; Fixed delivery mode
    
    ; Set vector
    or eax, esi         ; vector
    
    ; Write to ICR (Interrupt Command Register)
    ; APIC base is typically at 0xFEE00000
    mov rcx, 0xFEE00300 ; ICR high
    mov [rcx], eax
    mov rcx, 0xFEE00310 ; ICR low
    mov [rcx], eax
    
    ret

; Send IPI to all CPUs (broadcast)
; void send_ipi_all(u32 vector)
global send_ipi_all
send_ipi_all:
    mov eax, 0x00080000 ; All excluding self
    or eax, 0x00004000  ; Fixed delivery
    or eax, esi         ; vector
    
    mov rcx, 0xFEE00300 ; ICR high
    mov [rcx], eax
    mov rcx, 0xFEE00310 ; ICR low
    mov [rcx], eax
    
    ret

; Send IPI to all other CPUs (excluding self)
; void send_ipi_others(u32 vector)
global send_ipi_others
send_ipi_others:
    mov eax, 0x00080000 ; All excluding self
    or eax, 0x00004000  ; Fixed delivery
    or eax, esi         ; vector
    
    mov rcx, 0xFEE00300 ; ICR high
    mov [rcx], eax
    mov rcx, 0xFEE00310 ; ICR low
    mov [rcx], eax
    
    ret

; Get current CPU ID
; u32 get_cpu_id(void)
global get_cpu_id
get_cpu_id:
    ; Read APIC ID from APIC
    mov rcx, 0xFEE00020 ; APIC ID register
    mov eax, [rcx]
    shr eax, 24         ; APIC ID is in bits 24-31
    ret

; Get APIC base address
; u64 get_apic_base(void)
global get_apic_base
get_apic_base:
    ; Read from MSR
    mov ecx, 0x1B       ; IA32_APIC_BASE MSR
    rdmsr
    and eax, 0xFFFFF000 ; Base address (bits 12-35)
    shl rdx, 32
    or rax, rdx
    ret

; Enable local APIC
; void apic_enable(void)
global apic_enable
apic_enable:
    mov ecx, 0x1B       ; IA32_APIC_BASE MSR
    rdmsr
    or eax, 0x800       ; Enable APIC (bit 11)
    wrmsr
    ret

; Send EOI (End of Interrupt)
; void apic_eoi(void)
global apic_eoi
apic_eoi:
    mov dword [0xFEE000B0], 0 ; EOI register
    ret
