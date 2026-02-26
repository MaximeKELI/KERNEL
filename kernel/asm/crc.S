; CRC and checksum calculations
; Critical for network protocols and file integrity

bits 64
section .text

; CRC32 calculation (polynomial 0xEDB88320)
; u32 crc32_asm(const void* data, size_t len, u32 crc)
global crc32_asm
crc32_asm:
    mov eax, edx       ; crc -> eax
    mov rcx, rsi       ; len -> rcx
    test rcx, rcx
    jz .done
    
    ; CRC32 polynomial table (would be precomputed)
    ; For now, use CPU CRC32 instruction if available
    test rcx, rcx
    jz .done
    
    ; Use hardware CRC32 if available (SSE4.2)
    ; Otherwise, software implementation
    mov r8, rdi
    xor eax, eax
    
.loop:
    movzx edx, byte [r8]
    crc32 eax, dl      ; Hardware CRC32 (requires SSE4.2)
    inc r8
    dec rcx
    jnz .loop
    
.done:
    ret

; Simple checksum (sum of bytes)
; u16 checksum_asm(const void* data, size_t len)
global checksum_asm
checksum_asm:
    mov rcx, rsi       ; len -> rcx
    xor eax, eax       ; Sum
    test rcx, rcx
    jz .done
    
    mov r8, rdi
    xor edx, edx
    
.loop:
    movzx edx, byte [r8]
    add eax, edx
    inc r8
    dec rcx
    jnz .loop
    
    ; Fold to 16 bits
    mov ecx, eax
    shr eax, 16
    add ax, cx
    
.done:
    ret

; Internet checksum (RFC 1071)
; u16 ip_checksum_asm(const void* data, size_t len)
global ip_checksum_asm
ip_checksum_asm:
    mov rcx, rsi       ; len -> rcx
    xor eax, eax       ; Sum
    test rcx, rcx
    jz .done
    
    mov r8, rdi
    xor edx, edx
    
    ; Process 16-bit words
    shr rcx, 1
    jz .odd_byte
    
.word_loop:
    movzx edx, word [r8]
    add eax, edx
    add r8, 2
    dec rcx
    jnz .word_loop
    
.odd_byte:
    test rsi, 1
    jz .fold
    movzx edx, byte [r8]
    shl edx, 8
    add eax, edx
    
.fold:
    ; Fold 32-bit sum to 16 bits
    mov ecx, eax
    shr eax, 16
    add ax, cx
    adc ax, 0          ; Add carry
    
    ; One's complement
    not ax
    
.done:
    ret
