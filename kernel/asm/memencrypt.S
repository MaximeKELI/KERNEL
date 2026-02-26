; Memory encryption operations
; Support for AMD SME and Intel TME

bits 64
section .text

; Enable AMD SME (Secure Memory Encryption)
; void sme_enable(void)
global sme_enable
sme_enable:
    mov ecx, 0xC0010010 ; SYSCFG MSR
    rdmsr
    or eax, (1 << 23)   ; Set SME enable bit
    wrmsr
    ret

; Enable Intel TME (Total Memory Encryption)
; void tme_enable(void)
global tme_enable
tme_enable:
    mov ecx, 0x982      ; IA32_TME_ACTIVATE MSR
    rdmsr
    or eax, 1           ; Set TME enable bit
    wrmsr
    ret

; Get memory encryption status
; u32 get_memencrypt_status(void)
global get_memencrypt_status
get_memencrypt_status:
    ; Check for AMD SME
    mov ecx, 0xC0010010
    rdmsr
    test eax, (1 << 23)
    jnz .sme_enabled
    
    ; Check for Intel TME
    mov ecx, 0x982
    rdmsr
    test eax, 1
    jnz .tme_enabled
    
    xor eax, eax        ; Not enabled
    ret
    
.sme_enabled:
    mov eax, 1          ; SME
    ret
    
.tme_enabled:
    mov eax, 2          ; TME
    ret

; Set memory encryption key (AMD SME)
; void sme_set_key(u64 key)
global sme_set_key
sme_set_key:
    mov ecx, 0xC0010011 ; SMMASK MSR
    mov eax, edi        ; Low 32 bits
    mov edx, esi        ; High 32 bits
    wrmsr
    ret

; Get memory encryption key
; u64 sme_get_key(void)
global sme_get_key
sme_get_key:
    mov ecx, 0xC0010011 ; SMMASK MSR
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret
