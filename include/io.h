#ifndef IO_H
#define IO_H

#include "types.h"

/* Port I/O */
static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(u16 port, u8 value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(u16 port, u16 value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(u16 port, u32 value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/* Memory barriers */
static inline void mb(void) {
    asm volatile("mfence" ::: "memory");
}

static inline void rmb(void) {
    asm volatile("lfence" ::: "memory");
}

static inline void wmb(void) {
    asm volatile("sfence" ::: "memory");
}

/* CPUID */
static inline void cpuid(u32 leaf, u32* eax, u32* ebx, u32* ecx, u32* edx) {
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "a"(leaf));
}

/* MSR */
static inline u64 rdmsr(u32 msr) {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((u64)high << 32) | low;
}

static inline void wrmsr(u32 msr, u64 value) {
    u32 low = (u32)value;
    u32 high = (u32)(value >> 32);
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

#endif /* IO_H */
