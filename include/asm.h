#ifndef ASM_H
#define ASM_H

#include "types.h"

/* Atomic operations */
int atomic_cas(volatile u64* ptr, u64 old_val, u64 new_val);
u64 atomic_xchg(volatile u64* ptr, u64 new_val);
u64 atomic_fetch_add(volatile u64* ptr, u64 value);
u64 atomic_fetch_sub(volatile u64* ptr, u64 value);
u64 atomic_inc(volatile u64* ptr);
u64 atomic_dec(volatile u64* ptr);
void atomic_add(volatile u64* ptr, u64 value);
void atomic_sub(volatile u64* ptr, u64 value);
void atomic_bit_set(volatile u64* ptr, u32 bit);
void atomic_bit_clear(volatile u64* ptr, u32 bit);
u32 atomic_bit_test_set(volatile u64* ptr, u32 bit);
u32 atomic_bit_test_clear(volatile u64* ptr, u32 bit);

/* FPU/SSE/AVX operations */
void fpu_save(void* fpu_state);
void fpu_restore(void* fpu_state);
void fpu_init(void);
void sse_save(void* sse_state);
void sse_restore(void* sse_state);
void avx_save(void* avx_state);
void avx_restore(void* avx_state);

/* Optimized string operations */
void* memcpy_asm(void* dest, const void* src, size_t n);
void* memset_asm(void* s, int c, size_t n);
int memcmp_asm(const void* s1, const void* s2, size_t n);
size_t strlen_asm(const char* s);
int strcmp_asm(const char* s1, const char* s2);

/* CRC and checksum */
u32 crc32_asm(const void* data, size_t len, u32 crc);
u16 checksum_asm(const void* data, size_t len);
u16 ip_checksum_asm(const void* data, size_t len);

/* Cache control */
void clflush_asm(void* addr);
void clflushopt_asm(void* addr);
void prefetch_asm(void* addr, u32 hint);
void mfence_asm(void);
void lfence_asm(void);
void sfence_asm(void);
void wbinvd_asm(void);
void invd_asm(void);

/* TSC operations */
u64 rdtsc_asm(void);
u64 rdtscp_asm(u32* aux);
u64 rdtsc_pause_asm(void);
u64 tsc_calibrate_asm(void);

/* Bit operations */
u32 ffs_asm(u64 value);
u32 fls_asm(u64 value);
u32 popcount_asm(u64 value);
u32 clz_asm(u64 value);
u32 ctz_asm(u64 value);
u64 rol_asm(u64 value, u32 shift);
u64 ror_asm(u64 value, u32 shift);
u64 bit_reverse_asm(u64 value);

/* Context switch */
void context_switch_asm(void* from, void* to);

/* Page table operations */
u64 get_cr3(void);
void set_cr3(u64 cr3);
void invlpg_asm(void* addr);
void invlpg_all(void);
u64* get_pte_asm(void* virt, u64 cr3);
void set_pte_asm(u64* pte, u64 value);
u64 virt_to_phys_asm(void* virt, u64 cr3);

/* SMP operations */
void send_ipi(u32 cpu_id, u32 vector);
void send_ipi_all(u32 vector);
void send_ipi_others(u32 vector);
u32 get_cpu_id(void);
u64 get_apic_base(void);
void apic_enable(void);
void apic_eoi(void);

/* Retpoline (Spectre mitigation) */
void __x86_indirect_thunk_r11(void);
void __x86_indirect_thunk_rax(void);
void __x86_indirect_thunk_rdi(void);
void __x86_indirect_thunk_rsi(void);
void indirect_call_retpoline(void* target);
void indirect_jump_retpoline(void* target);

/* Lock-free data structures */
int lf_stack_push(volatile u64** head, u64* node);
u64* lf_stack_pop(volatile u64** head);
int lf_queue_enqueue(volatile u64** tail, u64* node);
u64* lf_queue_dequeue(volatile u64** head);
u64 lf_counter_inc(volatile u64* counter);
u64 lf_counter_dec(volatile u64* counter);

/* SIMD operations */
void simd_add_16(void* dst, const void* src1, const void* src2);
void simd_mul_16(void* dst, const void* src1, const void* src2);
u32 simd_cmp_16(const void* src1, const void* src2);
void simd_zero_16(void* dst);
void simd_copy_16(void* dst, const void* src);
u32 simd_sum_16(const void* src);
u8 simd_min_16(const void* src);
u8 simd_max_16(const void* src);

/* Exception fast paths */
int page_fault_fast(void* addr, u64 error_code);
int gp_fault_fast(void* addr, u64 error_code);
int div_zero_fast(void);
int invalid_opcode_fast(void);
int breakpoint_fast(void);

/* NUMA operations */
u32 numa_node_for_addr(void* addr);
u32 numa_distance(u32 node1, u32 node2);
void numa_prefetch(void* addr, u32 node);
void numa_mb(void);
u32 get_numa_node(void);

/* Power management */
void cpu_halt(void);
void cpu_halt_with_interrupts(void);
void cpu_deep_sleep(void);
u64 get_cpu_frequency(void);
void set_cpu_frequency(u32 freq_mhz);
u64 get_power_limit(void);
void set_power_limit(u64 limit);

/* Memory encryption */
void sme_enable(void);
void tme_enable(void);
u32 get_memencrypt_status(void);
void sme_set_key(u64 key);
u64 sme_get_key(void);

#endif /* ASM_H */
