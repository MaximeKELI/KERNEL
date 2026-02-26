#ifndef IO_H
#define IO_H

#include "types.h"

/* Port I/O */
static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    (void)port;  /* Suppress unused parameter warning */
    return ret;
}

static inline void outb(u16 port, u8 value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
    (void)port;
    (void)value;
}

static inline u16 inw(u16 port) {
    u16 ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    (void)port;
    return ret;
}

static inline void outw(u16 port, u16 value) {
    __asm__ __volatile__("outw %0, %1" : : "a"(value), "Nd"(port));
    (void)port;
    (void)value;
}

static inline u32 inl(u16 port) {
    u32 ret;
    __asm__ __volatile__("inl %1, %0" : "=a"(ret) : "Nd"(port));
    (void)port;
    return ret;
}

static inline void outl(u16 port, u32 value) {
    __asm__ __volatile__("outl %0, %1" : : "a"(value), "Nd"(port));
    (void)port;
    (void)value;
}

/* Memory barriers */
static inline void mb(void) {
    __asm__ __volatile__("mfence" ::: "memory");
}

static inline void rmb(void) {
    __asm__ __volatile__("lfence" ::: "memory");
}

static inline void wmb(void) {
    __asm__ __volatile__("sfence" ::: "memory");
}

/* CPUID */
static inline void cpuid(u32 leaf, u32* eax, u32* ebx, u32* ecx, u32* edx) {
    __asm__ __volatile__("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "a"(leaf));
}

/* MSR */
static inline u64 rdmsr(u32 msr) {
    u32 low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((u64)high << 32) | low;
}

static inline void wrmsr(u32 msr, u64 value) {
    u32 low = (u32)value;
    u32 high = (u32)(value >> 32);
    __asm__ __volatile__("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

#endif /* IO_H */
