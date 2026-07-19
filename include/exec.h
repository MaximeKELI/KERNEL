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

/* Program-header/entry info produced by the loader for the SysV auxv. */
typedef struct elf_load_info {
    u64 entry;
    u64 phdr;
    u64 phent;
    u64 phnum;
    u64 base;
} elf_load_info_t;
extern elf_load_info_t g_last_elf_info;

/* Load ELF at fixed user addresses; returns entry point */
int exec_load_elf(const void* elf_data, size_t size, u64* entry_out);

/* Apply R_X86_64_RELATIVE entries from a RELA table (static-PIE relocation). */
void exec_apply_rela(u64 bias, u64 rela_addr, u64 relasz, u64 relaent);

/* Resolve path to embedded nettest or return NULL */
const void* exec_resolve_path(const char* path, size_t* size_out);

/* True if a resolved blob came from the VFS (vs an embedded incbin blob). */
bool exec_blob_is_vfs(const void* blob);

/* Replace current task image and jump to user entry (no return) */
int exec_run_path(const char* path);

/* exec_run_path variant that lays down a SysV argv/envp/auxv stack. */
int exec_run_path_argv(const char* path, char* const argv[]);

/* Jump to already-loaded user entry */
void exec_jump_user(u64 entry) __attribute__((noreturn));

/* Enter ring 3 via iretq (CS=0x23, SS=0x2b) */
void exec_iretq_user(u64 rip, u64 rsp, u64 rflags) __attribute__((noreturn));

#endif /* EXEC_H */
