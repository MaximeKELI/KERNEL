#ifndef EXEC_H
#define EXEC_H

#include "types.h"

#define USER_LOAD_ADDR  0x400000UL
#define USER_STACK_TOP  0x00600000UL
#define USER_STACK_SIZE (64 * 1024)

/* Load ELF at fixed user addresses; returns entry point */
int exec_load_elf(const void* elf_data, size_t size, u64* entry_out);

/* Resolve path to embedded nettest or return NULL */
const void* exec_resolve_path(const char* path, size_t* size_out);

/* Replace current task image and jump to user entry (no return) */
int exec_run_path(const char* path);

/* Jump to already-loaded user entry */
void exec_jump_user(u64 entry) __attribute__((noreturn));

#endif /* EXEC_H */
