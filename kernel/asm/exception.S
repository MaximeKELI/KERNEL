; Exception handling fast paths
; Optimized exception handlers for common cases

bits 64
section .text

; Fast page fault handler
; Returns: 0 if handled, -1 if needs full handler
global page_fault_fast
page_fault_fast:
    ; Check if it's a write fault on present page (copy-on-write)
    test rsi, 0x02      ; Write bit in error code
    jz .slow
    test rsi, 0x01      ; Present bit
    jz .slow
    
    ; Fast path: handle copy-on-write
    ; Would implement COW handling here
    ; For now, fall through to slow path
.slow:
    mov eax, -1
    ret

; Fast general protection fault handler
; Returns: 0 if handled, -1 if needs full handler
global gp_fault_fast
gp_fault_fast:
    ; Check for common cases (null pointer, etc.)
    test rsi, rsi
    jz .null_ptr
    
    ; Slow path
    mov eax, -1
    ret
    
.null_ptr:
    ; Handle null pointer access
    ; Would log and kill process
    mov eax, 0
    ret

; Fast divide by zero handler
global div_zero_fast
div_zero_fast:
    ; Return error code
    mov eax, -1
    ret

; Fast invalid opcode handler
global invalid_opcode_fast
invalid_opcode_fast:
    ; Check for UD2 (undefined instruction - used for breakpoints)
    ; Would decode instruction here
    mov eax, -1
    ret

; Fast breakpoint handler
global breakpoint_fast
breakpoint_fast:
    ; Handle INT3 breakpoint
    ; Would call debugger or continue
    mov eax, 0
    ret
