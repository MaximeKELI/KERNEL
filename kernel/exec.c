#include "exec.h"
#include "elf.h"
#include "memory.h"
#include "process.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "validate.h"
#include "gdt.h"

extern char nettest_bin_start[];
extern char nettest_bin_end[];
extern char sh_bin_start[];
extern char sh_bin_end[];

#define PT_LOAD 1

int exec_load_elf(const void* elf_data, size_t size, u64* entry_out) {
    if (!elf_data || !entry_out || elf_validate(elf_data, size) < 0) {
        return -1;
    }

    const elf_header_t* ehdr = (const elf_header_t*)elf_data;
    const elf_phdr_t* phdr = (const elf_phdr_t*)((const u8*)elf_data + ehdr->e_phoff);

    for (u16 i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        u64 vaddr = phdr[i].p_vaddr;
        size_t memsz = phdr[i].p_memsz;
        size_t filesz = phdr[i].p_filesz;
        u64 flags = phdr[i].p_flags;

        if (vaddr < USER_LOAD_ADDR || vaddr + memsz > USER_STACK_TOP) {
            DEBUG_ERROR("ELF segment outside user range");
            return -1;
        }

        u64 page_flags = PAGE_PRESENT | PAGE_USER;
        if (flags & 0x2) {
            page_flags |= PAGE_WRITABLE;
        }

        u64 page_start = vaddr & ~(PAGE_SIZE - 1);
        u64 page_end = (vaddr + memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        for (u64 pa = page_start; pa < page_end; pa += PAGE_SIZE) {
            void* phys = pmm_alloc(1);
            if (!phys) {
                return -1;
            }
            memset(phys, 0, PAGE_SIZE);
            if (!vmm_map_page((void*)pa, phys, page_flags)) {
                return -1;
            }
        }

        u8* dest = (u8*)vaddr;
        const u8* src = (const u8*)elf_data + phdr[i].p_offset;
        if (filesz > 0) {
            memcpy(dest, src, filesz);
        }
        if (memsz > filesz) {
            memset(dest + filesz, 0, memsz - filesz);
        }
    }

    *entry_out = ehdr->e_entry;
    return 0;
}

static u8 exec_file_buf[256 * 1024];

const void* exec_resolve_path(const char* path, size_t* size_out) {
    if (!path || !size_out) {
        return NULL;
    }

    if (strcmp(path, "/nettest") == 0 || strcmp(path, "/boot/nettest") == 0 ||
        strcmp(path, "nettest") == 0) {
        *size_out = (size_t)(nettest_bin_end - nettest_bin_start);
        return nettest_bin_start;
    }
    if (strcmp(path, "/sh") == 0 || strcmp(path, "/boot/sh") == 0 ||
        strcmp(path, "sh") == 0) {
        *size_out = (size_t)(sh_bin_end - sh_bin_start);
        return sh_bin_start;
    }

    extern ssize_t vfs_read_path(const char* path, void* buf, size_t count);
    ssize_t n = vfs_read_path(path, exec_file_buf, sizeof(exec_file_buf));
    if (n > 64 && exec_file_buf[0] == 0x7F && exec_file_buf[1] == 'E' &&
        exec_file_buf[2] == 'L' && exec_file_buf[3] == 'F') {
        *size_out = (size_t)n;
        return exec_file_buf;
    }
    return NULL;
}

void exec_jump_user(u64 entry) {
    gdt_init_user_segments();
    u64 stack = USER_STACK_TOP - 16;
    u64 rflags = 0x202;
    exec_iretq_user(entry, stack, rflags);
}

int exec_run_path(const char* path) {
    size_t size = 0;
    const void* blob = exec_resolve_path(path, &size);
    if (!blob) {
        return -1;
    }

    u64 entry = 0;
    if (exec_load_elf(blob, size, &entry) < 0) {
        return -1;
    }

    printk("[exec] %s ring3 entry=0x%llx (%zu bytes)\n",
           path, (unsigned long long)entry, size);
    exec_jump_user(entry);
    return 0;
}
