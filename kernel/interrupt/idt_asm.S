; IDT assembly stubs for interrupt handlers

bits 64
section .text

; Macro to create ISR stub
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0          ; Dummy error code
    push %1         ; Interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1         ; Interrupt number (error code already on stack)
    jmp isr_common
%endmacro

; Exception handlers
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 16
ISR_ERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20

; IRQ handlers
%macro IRQ 1
global irq%1
irq%1:
    push 0
    push %1 + 32
    jmp isr_common
%endmacro

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

; System call handler
global syscall_handler_asm
syscall_handler_asm:
    push 0
    push 0x80
    jmp isr_common

; Common interrupt handler
extern interrupt_handler
isr_common:
    ; Save all general-purpose registers (order must match interrupt_frame_t)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; rbp := pointer to the saved GP frame (interrupt_frame_t). rbp is callee-
    ; saved, so the C handler preserves it across the call. The interrupted rbp
    ; value is already safe on the stack and is restored by "pop rbp" below.
    mov rbp, rsp

    ; Preserve the interrupted task's FPU/SSE state BEFORE any C code runs, so a
    ; preemptive context switch inside interrupt_handler cannot corrupt it. The
    ; 512-byte fxsave area lives on this task's kernel stack (below the frame);
    ; because we always return to this exact frame, fxrstor restores it verbatim.
    mov rdi, [rbp + 15*8]      ; Interrupt number
    mov rsi, [rbp + 16*8]      ; Error code
    mov rdx, rbp               ; interrupt_frame_t*
    sub rsp, 512
    and rsp, -16               ; fxsave requires a 16-byte aligned operand
    fxsave [rsp]
    call interrupt_handler
    fxrstor [rsp]
    mov rsp, rbp               ; discard fxsave scratch, back to the GP frame

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Remove error code and interrupt number
    add rsp, 16

    iretq
