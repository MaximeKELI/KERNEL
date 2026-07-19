; Boot code for 64-bit kernel
; Multiboot2 compliant entry: GRUB loads us in 32-bit protected mode,
; we set up 4-level paging, switch the CPU to long mode, then call kernel_main.

bits 32
section .multiboot

; Multiboot2 header (framebuffer request 640x480x32)
align 8
multiboot_header_start:
    dd 0xe85250d6
    dd 0
    dd multiboot_header_end - multiboot_header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start))

    align 8
    dw 5
    dw 0
    dd 20
    dd 640
    dd 480
    dd 32

    align 8
    dw 0
    dw 0
    dd 8
multiboot_header_end:

; --- Page tables and stack (uninitialised, 4 KiB aligned) ---
section .bss
alignb 4096
p4_table:                       ; PML4
    resb 4096
p3_table:                       ; PDPT
    resb 4096
p2_tables:                      ; 4 page directories -> identity map first 4 GiB
    resb 4096 * 4
align 16
global kernel_stack_top
kernel_stack_bottom:
    resb 16384
kernel_stack_top:

section .text
global _start
extern kernel_main

; -----------------------------------------------------------------------------
; 32-bit entry point (from GRUB)
;   eax = Multiboot2 magic (0x36d76289)
;   ebx = physical address of the Multiboot2 info structure
; -----------------------------------------------------------------------------
bits 32
_start:
    cld
    mov esp, kernel_stack_top

    ; Preserve boot arguments in callee-saved registers so they survive the
    ; page-table setup calls: kernel_main(u64 magic, u64 mb_info) -> rdi, rsi.
    mov edi, eax                ; magic  -> rdi (zero-extended)
    mov esi, ebx                ; mb_info -> rsi (zero-extended)

    cmp eax, 0x36d76289
    jne .error

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

.error:
    ; "EROR" (white on red) in the VGA text buffer, then halt.
    mov dword [0xb8000], 0x4f524f45
    cli
.error_hang:
    hlt
    jmp .error_hang

; -----------------------------------------------------------------------------
; Build identity-mapping page tables for the first 4 GiB using 2 MiB pages.
; -----------------------------------------------------------------------------
setup_page_tables:
    ; PML4[0] -> PDPT
    mov eax, p3_table
    or eax, 0b11                ; present + writable
    mov [p4_table], eax

    ; PDPT[0..3] -> the four page directories
    mov eax, p2_tables
    or eax, 0b11
    mov [p3_table + 0*8], eax
    add eax, 4096
    mov [p3_table + 1*8], eax
    add eax, 4096
    mov [p3_table + 2*8], eax
    add eax, 4096
    mov [p3_table + 3*8], eax

    ; Fill 2048 PD entries, each a 2 MiB identity-mapped huge page.
    xor ecx, ecx
.map_p2:
    mov eax, 0x200000           ; 2 MiB
    mul ecx                     ; edx:eax = 2 MiB * ecx (physical address)
    or eax, 0b10000011          ; present + writable + huge (PS)
    mov [p2_tables + ecx*8], eax
    inc ecx
    cmp ecx, 2048
    jne .map_p2
    ret

; -----------------------------------------------------------------------------
; Enable PAE, set EFER.LME, load CR3, then turn on paging to activate long mode.
; -----------------------------------------------------------------------------
enable_paging:
    mov eax, p4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5             ; CR4.PAE
    or eax, 1 << 9             ; CR4.OSFXSR      (enable SSE / FXSAVE)
    or eax, 1 << 10            ; CR4.OSXMMEXCPT  (SSE #XM exceptions)
    mov cr4, eax

    mov ecx, 0xC0000080        ; EFER MSR
    rdmsr
    or eax, 1 << 8             ; EFER.LME
    wrmsr

    mov eax, cr0
    and eax, 0xFFFFFFFB        ; clear CR0.EM  (no x87/SSE emulation)
    or eax, 1 << 1             ; CR0.MP
    or eax, 1 << 31            ; CR0.PG
    or eax, 1 << 0             ; CR0.PE (already set by GRUB)
    mov cr0, eax
    ret

; -----------------------------------------------------------------------------
; 64-bit GDT: null, 64-bit code, data.
; -----------------------------------------------------------------------------
section .rodata
align 8
gdt64:
    dq 0                                                    ; null descriptor
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)                ; exec, S, present, long-mode
.data: equ $ - gdt64
    dq (1<<41) | (1<<44) | (1<<47)                          ; writable, S, present
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; -----------------------------------------------------------------------------
; 64-bit entry: load segments and jump into C.
; -----------------------------------------------------------------------------
section .text
bits 64
long_mode_start:
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; rdi = magic, rsi = mb_info (set in 32-bit code, upper bits zeroed)
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
