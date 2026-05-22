; Boot code for 64-bit kernel
; Multiboot2 compliant bootloader entry

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

section .text
global _start

extern kernel_main

section .bss
align 16
global kernel_stack_top
kernel_stack_bottom:
    resb 8192
kernel_stack_top:

_start:
    cld
    mov edi, ebx
    mov esi, eax
    cmp esi, 0x36d76289
    jne .error
    mov esp, kernel_stack_top
    push 0
    popf
    call setup_gdt
    call setup_paging
    call enable_long_mode
    jmp CODE_SEG64:long_mode_start

.error:
    mov dword [0xb8000], 0x4f524f45
    hlt

setup_gdt:
    lgdt [gdt_descriptor]
    ret

setup_paging:
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    mov edi, cr3
    mov dword [edi], 0x1003
    add edi, 0x1000
    mov dword [edi], 0x2003
    ret

enable_long_mode:
    mov eax, 10100000b
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov edi, 0x1000
    mov eax, edi
    add eax, 0x1000
    or dword [edi], 0x00
    or dword [eax], 0x20
    mov eax, cr4
    bts eax, 5
    mov cr4, eax
    mov ecx, 0xC0000082
    mov edx, long_mode_start
    wrmsr
    ret

bits 64
long_mode_start:
    mov ax, DATA_SEG64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rdi, rbx
    mov rsi, rax
    extern kernel_main
    call kernel_main
    cli
    hlt

bits 32
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt
    dd 0

CODE_SEG64 equ 0x08
DATA_SEG64 equ 0x10
