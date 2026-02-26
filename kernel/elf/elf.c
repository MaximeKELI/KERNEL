#include "elf.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"

#define PT_LOAD 1

int elf_validate(const void* elf_data, size_t size) {
    if (!elf_data || size < sizeof(elf_header_t)) {
        return -1;
    }
    
    const elf_header_t* ehdr = (const elf_header_t*)elf_data;
    
    /* Check magic */
    if (*(u32*)ehdr->e_ident != ELF_MAGIC) {
        DEBUG_ERROR("Invalid ELF magic");
        return -1;
    }
    
    /* Check class (64-bit) */
    if (ehdr->e_ident[4] != 2) {
        DEBUG_ERROR("Not a 64-bit ELF");
        return -1;
    }
    
    /* Check endianness (little) */
    if (ehdr->e_ident[5] != 1) {
        DEBUG_ERROR("Not little-endian ELF");
        return -1;
    }
    
    /* Check machine (x86_64) */
    if (ehdr->e_machine != 0x3E) {
        DEBUG_ERROR("Not x86_64 ELF");
        return -1;
    }
    
    return 0;
}

int elf_load(const void* elf_data, size_t size, void** entry_point) {
    if (elf_validate(elf_data, size) < 0) {
        return -1;
    }
    
    const elf_header_t* ehdr = (const elf_header_t*)elf_data;
    
    /* Load program segments */
    const elf_phdr_t* phdr = (const elf_phdr_t*)((u8*)elf_data + ehdr->e_phoff);
    
    for (u16 i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            /* Allocate memory */
            void* vaddr = (void*)phdr[i].p_vaddr;
            size_t memsz = phdr[i].p_memsz;
            
            /* Allocate pages */
            u64 pages = (memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            void* allocated = vmm_alloc_pages(pages);
            
            if (!allocated) {
                DEBUG_ERROR("Failed to allocate memory for ELF segment");
                return -1;
            }
            
            /* Copy segment data */
            const void* src = (u8*)elf_data + phdr[i].p_offset;
            memcpy(allocated, src, phdr[i].p_filesz);
            
            /* Zero BSS */
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                memset((u8*)allocated + phdr[i].p_filesz, 0,
                       phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }
    
    *entry_point = (void*)ehdr->e_entry;
    
    DEBUG_INFO("ELF loaded: entry=0x%p", *entry_point);
    return 0;
}
