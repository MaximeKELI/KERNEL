#ifndef ELF_H
#define ELF_H

#include "types.h"

/* ELF header */
typedef struct __packed {
    unsigned char e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} elf_header_t;

/* Program header */
typedef struct __packed {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} elf_phdr_t;

/* Section header */
typedef struct __packed {
    u32 sh_name;
    u32 sh_type;
    u64 sh_flags;
    u64 sh_addr;
    u64 sh_offset;
    u64 sh_size;
    u32 sh_link;
    u32 sh_info;
    u64 sh_addralign;
    u64 sh_entsize;
} elf_shdr_t;

/* ELF magic */
#define ELF_MAGIC 0x464C457F

/* Load ELF executable */
int elf_load(const void* elf_data, size_t size, void** entry_point);

/* Validate ELF */
int elf_validate(const void* elf_data, size_t size);

#endif /* ELF_H */
