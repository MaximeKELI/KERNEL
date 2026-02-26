; Lock-free data structures
; High-performance queues and stacks

bits 64
section .text

; Lock-free stack push
; int lf_stack_push(volatile u64** head, u64* node)
; Returns: 0 on success, -1 on failure
global lf_stack_push
lf_stack_push:
    mov r8, [rdi]       ; Load current head
.loop:
    mov [rsi], r8       ; node->next = head
    mov rax, rsi        ; node -> rax
    lock cmpxchg [rdi], r8  ; Try to update head
    jne .loop           ; Retry if failed
    xor eax, eax        ; Success
    ret

; Lock-free stack pop
; u64* lf_stack_pop(volatile u64** head)
; Returns: node pointer or NULL
global lf_stack_pop
lf_stack_pop:
    mov r8, [rdi]       ; Load current head
.loop:
    test r8, r8
    jz .empty
    mov r9, [r8]        ; Load next pointer
    mov rax, r8         ; head -> rax
    lock cmpxchg [rdi], r9  ; Try to update head
    jne .loop           ; Retry if failed
    mov rax, r8         ; Return popped node
    ret
.empty:
    xor rax, rax
    ret

; Lock-free queue enqueue
; int lf_queue_enqueue(volatile u64** tail, u64* node)
global lf_queue_enqueue
lf_queue_enqueue:
    mov qword [rsi], 0  ; node->next = NULL
    mov r8, rsi          ; node -> r8
    xchg [rdi], r8       ; Exchange with tail
    test r8, r8
    jz .first           ; First node
    mov [r8], rsi        ; old_tail->next = node
    xor eax, eax
    ret
.first:
    ; First node - need to update head too
    ; (would need head pointer as parameter)
    xor eax, eax
    ret

; Lock-free queue dequeue
; u64* lf_queue_dequeue(volatile u64** head)
global lf_queue_dequeue
lf_queue_dequeue:
    mov r8, [rdi]       ; Load current head
.loop:
    test r8, r8
    jz .empty
    mov r9, [r8]        ; Load next pointer
    mov rax, r8         ; head -> rax
    lock cmpxchg [rdi], r9  ; Try to update head
    jne .loop           ; Retry if failed
    mov rax, r8         ; Return dequeued node
    ret
.empty:
    xor rax, rax
    ret

; Lock-free counter increment
; u64 lf_counter_inc(volatile u64* counter)
global lf_counter_inc
lf_counter_inc:
    lock inc qword [rdi]
    mov rax, [rdi]
    ret

; Lock-free counter decrement
; u64 lf_counter_dec(volatile u64* counter)
global lf_counter_dec
lf_counter_dec:
    lock dec qword [rdi]
    mov rax, [rdi]
    ret
