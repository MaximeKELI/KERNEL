; Retpoline - Spectre mitigation
; Prevents speculative execution attacks

bits 64
section .text

; Retpoline thunk for indirect calls
; This prevents speculative execution from following indirect branches
global __x86_indirect_thunk_r11
__x86_indirect_thunk_r11:
    call .capture_spec
.spec_trap:
    pause
    lfence
    jmp .spec_trap
.capture_spec:
    mov [rsp], r11
    ret

global __x86_indirect_thunk_rax
__x86_indirect_thunk_rax:
    call .capture_spec
.spec_trap:
    pause
    lfence
    jmp .spec_trap
.capture_spec:
    mov [rsp], rax
    ret

global __x86_indirect_thunk_rdi
__x86_indirect_thunk_rdi:
    call .capture_spec
.spec_trap:
    pause
    lfence
    jmp .spec_trap
.capture_spec:
    mov [rsp], rdi
    ret

global __x86_indirect_thunk_rsi
__x86_indirect_thunk_rsi:
    call .capture_spec
.spec_trap:
    pause
    lfence
    jmp .spec_trap
.capture_spec:
    mov [rsp], rsi
    ret

; Generic indirect call with retpoline
; void indirect_call_retpoline(void* target)
global indirect_call_retpoline
indirect_call_retpoline:
    mov r11, rdi
    call __x86_indirect_thunk_r11
    ret

; Indirect jump with retpoline
; void indirect_jump_retpoline(void* target)
global indirect_jump_retpoline
indirect_jump_retpoline:
    mov r11, rdi
    jmp __x86_indirect_thunk_r11
