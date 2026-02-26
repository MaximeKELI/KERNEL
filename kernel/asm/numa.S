; NUMA operations
; NUMA-aware memory access and topology

bits 64
section .text

; Get NUMA node for address
; u32 numa_node_for_addr(void* addr)
global numa_node_for_addr
numa_node_for_addr:
    ; Use CPUID to get NUMA information
    mov eax, 0x8000001E ; Extended topology enumeration
    cpuid
    ; Node ID is in ECX bits 8-15
    mov eax, ecx
    shr eax, 8
    and eax, 0xFF
    ret

; Get NUMA distance between nodes
; u32 numa_distance(u32 node1, u32 node2)
global numa_distance
numa_distance:
    ; Would read from ACPI SLIT table
    ; For now, return 10 (local) or 20 (remote)
    cmp edi, esi
    je .local
    mov eax, 20
    ret
.local:
    mov eax, 10
    ret

; Prefetch to NUMA node
; void numa_prefetch(void* addr, u32 node)
global numa_prefetch
numa_prefetch:
    ; Prefetch data to specified NUMA node
    prefetcht0 [rdi]
    ; Would use node-specific prefetch hints if available
    ret

; Memory barrier for NUMA
; void numa_mb(void)
global numa_mb
numa_mb:
    mfence               ; Full memory barrier
    ret

; Get current NUMA node
; u32 get_numa_node(void)
global get_numa_node
get_numa_node:
    ; Read from CPUID
    mov eax, 0x8000001E
    cpuid
    mov eax, ecx
    shr eax, 8
    and eax, 0xFF
    ret
