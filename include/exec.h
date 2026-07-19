#ifndef EXEC_H
#define EXEC_H

#include "types.h"

/*
 * User address space lives at 1 GiB, well clear of the kernel image + heap,
 * which occupy low physical/virtual memory (kernel .text/.bss up to ~4.3 MiB,
 * heap 2..12 MiB, all identity-mapped). Loading a user ELF at the old 0x400000
 * base remapped those very pages and corrupted the running kernel (#GP). The
 * loader maps this window explicitly, so being past the identity map is fine.
 */
#define USER_LOAD_ADDR  0x40000000UL              /* 1 GiB */
#define USER_STACK_TOP  0x40400000UL              /* 1 GiB + 4 MiB */
#define USER_STACK_SIZE (64 * 1024)

/* Load ELF at fixed user addresses; returns entry point */
int exec_load_elf(const void* elf_data, size_t size, u64* entry_out);

/* Resolve path to embedded nettest or return NULL */
const void* exec_resolve_path(const char* path, size_t* size_out);

/* Replace current task image and jump to user entry (no return) */
int exec_run_path(const char* path);

/* Jump to already-loaded user entry */
void exec_jump_user(u64 entry) __attribute__((noreturn));

/* Enter ring 3 via iretq (CS=0x23, SS=0x2b) */
void exec_iretq_user(u64 rip, u64 rsp, u64 rflags) __attribute__((noreturn));

#endif /* EXEC_H */
