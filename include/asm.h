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

#endif /* ASM_H */
