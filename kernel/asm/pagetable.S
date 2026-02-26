; Page table manipulation
; Direct manipulation of page tables for performance

bits 64
section .text

; Get CR3 (current page table base)
; u64 get_cr3(void)
global get_cr3
get_cr3:
    mov rax, cr3
    ret

; Set CR3 (switch page tables)
; void set_cr3(u64 cr3)
global set_cr3
set_cr3:
    mov cr3, rdi
    ret

; Invalidate TLB entry for address
; void invlpg_asm(void* addr)
global invlpg_asm
invlpg_asm:
    invlpg [rdi]
    ret

; Invalidate entire TLB
; void invlpg_all(void)
global invlpg_all
invlpg_all:
    mov rax, cr3
    mov cr3, rax        ; Reload CR3 flushes entire TLB
    ret

; Get page table entry (walk page tables)
; u64* get_pte_asm(void* virt, u64 cr3)
global get_pte_asm
get_pte_asm:
    mov rax, rsi        ; cr3 -> rax (PML4 base)
    mov rcx, rdi        ; virt -> rcx
    
    ; Extract indices
    mov rdx, rcx
    shr rdx, 39
    and rdx, 0x1FF      ; PML4 index
    mov rax, [rax + rdx * 8]
    test rax, 1         ; Check present bit
    jz .not_present
    and rax, ~0xFFF     ; Clear flags
    mov r8, rcx
    shr r8, 30
    and r8, 0x1FF       ; PDPT index
    mov rax, [rax + r8 * 8]
    test rax, 1
    jz .not_present
    and rax, ~0xFFF
    mov r8, rcx
    shr r8, 21
    and r8, 0x1FF       ; PD index
    mov rax, [rax + r8 * 8]
    test rax, 1
    jz .not_present
    and rax, ~0xFFF
    mov r8, rcx
    shr r8, 12
    and r8, 0x1FF       ; PT index
    lea rax, [rax + r8 * 8]
    ret
    
.not_present:
    xor rax, rax
    ret

; Set page table entry
; void set_pte_asm(u64* pte, u64 value)
global set_pte_asm
set_pte_asm:
    mov [rdi], rsi
    mfence               ; Ensure write is visible
    ret

; Get physical address from virtual
; u64 virt_to_phys_asm(void* virt, u64 cr3)
global virt_to_phys_asm
virt_to_phys_asm:
    push rdi
    push rsi
    call get_pte_asm
    pop rsi
    pop rdi
    test rax, rax
    jz .not_mapped
    mov rax, [rax]
    and rax, ~0xFFF      ; Clear flags
    ret
.not_mapped:
    xor rax, rax
    ret
