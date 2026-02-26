; Boot code for 64-bit kernel
; Multiboot2 compliant bootloader entry

bits 32
section .multiboot

; Multiboot2 header
align 8
multiboot_header_start:
    dd 0xe85250d6                ; Magic number
    dd 0                          ; Architecture (i386)
    dd multiboot_header_end - multiboot_header_start ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start)) ; Checksum

    ; End tag
    align 8
    dw 0    ; Type
    dw 0    ; Flags
    dd 8    ; Size
multiboot_header_end:

section .text
global _start

extern kernel_main
extern kernel_stack_top

_start:
    ; Clear direction flag
    cld

    ; Save multiboot2 info
    mov edi, ebx  ; Multiboot2 info structure pointer
    mov esi, eax  ; Magic number

    ; Check if we have multiboot2
    cmp esi, 0x36d76289
    jne .error

    ; Setup stack
    mov esp, kernel_stack_top

    ; Clear EFLAGS
    push 0
    popf

    ; Setup GDT
    call setup_gdt

    ; Setup paging and switch to long mode
    call setup_paging
    call enable_long_mode

    ; Jump to 64-bit code
    jmp CODE_SEG64:long_mode_start

.error:
    ; Error: not booted by multiboot2
    mov dword [0xb8000], 0x4f524f45  ; "ERROR"
    hlt

; Setup GDT
setup_gdt:
    lgdt [gdt_descriptor]
    ret

; Setup paging (identity map first 2MB)
setup_paging:
    ; Clear page tables
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    mov edi, cr3

    ; PML4
    mov dword [edi], 0x2003  ; Point to PDPT, present + writable
    add edi, 0x1000

    ; PDPT
    mov dword [edi], 0x3003  ; Point to PD, present + writable
    add edi, 0x1000

    ; PD (2MB pages)
    mov dword [edi], 0x4003  ; Point to PT, present + writable
    add edi, 0x1000

    ; PT (4KB pages for first 2MB)
    mov ebx, 0x00000003  ; Present + writable
    mov ecx, 512

.set_entry:
    mov dword [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .set_entry

    ret

; Enable long mode
enable_long_mode:
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

bits 64
long_mode_start:
    ; Setup segment registers
    mov ax, DATA_SEG64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup stack
    mov rsp, kernel_stack_top

    ; Clear RFLAGS
    push 0
    popf

    ; Call kernel main
    mov rdi, rsi  ; Multiboot2 magic
    mov rsi, rdi  ; Multiboot2 info
    call kernel_main

    ; Halt if kernel returns
    cli
    hlt

; GDT
section .rodata
align 8
gdt_start:
    dq 0  ; Null descriptor

gdt_code32:
    dw 0xFFFF      ; Limit (bits 0-15)
    dw 0           ; Base (bits 0-15)
    db 0           ; Base (bits 16-23)
    db 10011010b   ; Access byte
    db 11001111b   ; Flags + Limit (bits 16-19)
    db 0           ; Base (bits 24-31)

gdt_data32:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

gdt_code64:
    dw 0           ; Limit
    dw 0           ; Base
    db 0           ; Base
    db 10011010b   ; Access
    db 10100000b   ; Flags
    db 0           ; Base

gdt_data64:
    dw 0
    dw 0
    db 0
    db 10010010b
    db 10100000b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dq gdt_start

CODE_SEG64 equ gdt_code64 - gdt_start
DATA_SEG64 equ gdt_data64 - gdt_start
