; Atomic operations for x86_64
; Critical for lock-free algorithms and synchronization

bits 64
section .text

; Atomic compare and swap
; int atomic_cas(volatile u64* ptr, u64 old_val, u64 new_val)
; Returns: 1 if swapped, 0 otherwise
global atomic_cas
atomic_cas:
    mov rax, rsi        ; old_val -> rax
    mov rcx, rdx        ; new_val -> rcx
    lock cmpxchg [rdi], rcx
    sete al            ; Set al to 1 if equal (swapped), 0 otherwise
    movzx eax, al
    ret

; Atomic exchange
; u64 atomic_xchg(volatile u64* ptr, u64 new_val)
global atomic_xchg
atomic_xchg:
    mov rax, rsi       ; new_val -> rax
    xchg [rdi], rax    ; Exchange atomically
    ret

; Atomic fetch and add
; u64 atomic_fetch_add(volatile u64* ptr, u64 value)
global atomic_fetch_add
atomic_fetch_add:
    mov rax, rsi       ; value -> rax
    lock xadd [rdi], rax
    ret

; Atomic fetch and subtract
; u64 atomic_fetch_sub(volatile u64* ptr, u64 value)
global atomic_fetch_sub
atomic_fetch_sub:
    mov rax, rsi       ; value -> rax
    neg rax            ; Negate for subtraction
    lock xadd [rdi], rax
    ret

; Atomic increment
; u64 atomic_inc(volatile u64* ptr)
global atomic_inc
atomic_inc:
    lock inc qword [rdi]
    mov rax, [rdi]
    ret

; Atomic decrement
; u64 atomic_dec(volatile u64* ptr)
global atomic_dec
atomic_dec:
    lock dec qword [rdi]
    mov rax, [rdi]
    ret

; Atomic add
; void atomic_add(volatile u64* ptr, u64 value)
global atomic_add
atomic_add:
    lock add [rdi], rsi
    ret

; Atomic subtract
; void atomic_sub(volatile u64* ptr, u64 value)
global atomic_sub
atomic_sub:
    lock sub [rdi], rsi
    ret

; Atomic bit set
; void atomic_bit_set(volatile u64* ptr, u32 bit)
global atomic_bit_set
atomic_bit_set:
    mov rax, 1
    mov rcx, rsi       ; bit position
    shl rax, cl        ; Shift 1 to bit position
    lock or [rdi], rax
    ret

; Atomic bit clear
; void atomic_bit_clear(volatile u64* ptr, u32 bit)
global atomic_bit_clear
atomic_bit_clear:
    mov rax, 1
    mov rcx, rsi       ; bit position
    shl rax, cl        ; Shift 1 to bit position
    not rax            ; Invert for AND
    lock and [rdi], rax
    ret

; Atomic bit test and set
; u32 atomic_bit_test_set(volatile u64* ptr, u32 bit)
global atomic_bit_test_set
atomic_bit_test_set:
    mov rax, 1
    mov rcx, rsi       ; bit position
    shl rax, cl        ; Shift 1 to bit position
    lock bts [rdi], rsi
    setc al            ; Set al to 1 if bit was set, 0 otherwise
    movzx eax, al
    ret

; Atomic bit test and clear
; u32 atomic_bit_test_clear(volatile u64* ptr, u32 bit)
global atomic_bit_test_clear
atomic_bit_test_clear:
    lock btr [rdi], rsi
    setc al            ; Set al to 1 if bit was set, 0 otherwise
    movzx eax, al
    ret
