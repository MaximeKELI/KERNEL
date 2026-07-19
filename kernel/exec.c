#include "exec.h"
#include "elf.h"
#include "memory.h"
#include "mm.h"
#include "process.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "validate.h"
#include "gdt.h"
#include "tss.h"
#include "scheduler.h"
#include "vdso.h"
#include "fs/vfs.h"

extern char nettest_bin_start[];
extern char nettest_bin_end[];
extern char sh_bin_start[];
extern char sh_bin_end[];

#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_PHDR    6
#define ET_EXEC    2
#define ET_DYN     3

/* Dynamic-section tags (subset). */
#define DT_NULL     0
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define R_X86_64_RELATIVE 8

typedef struct __packed { i64 d_tag; u64 d_val; } elf_dyn_t;
typedef struct __packed { u64 r_offset; u64 r_info; i64 r_addend; } elf_rela_t;

/* Filled by exec_load_elf so exec can build a proper SysV auxv. */
elf_load_info_t g_last_elf_info;

/*
 * Apply R_X86_64_RELATIVE relocations for a static-PIE binary. The dynamic
 * section is walked to find the RELA table; each RELATIVE entry patches an
 * absolute pointer to (load_bias + addend). Enough to run musl static-PIE.
 */
static void exec_apply_relocations(const elf_header_t* ehdr, const void* elf_data,
                                   u64 bias) {
    const elf_phdr_t* phdr = (const elf_phdr_t*)((const u8*)elf_data + ehdr->e_phoff);
    u64 rela = 0, relasz = 0, relaent = sizeof(elf_rela_t);

    for (u16 i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_DYNAMIC) {
            continue;
        }
        const elf_dyn_t* dyn = (const elf_dyn_t*)(bias + phdr[i].p_vaddr);
        for (; dyn->d_tag != DT_NULL; dyn++) {
            if (dyn->d_tag == DT_RELA)    rela = bias + dyn->d_val;
            else if (dyn->d_tag == DT_RELASZ)  relasz = dyn->d_val;
            else if (dyn->d_tag == DT_RELAENT) relaent = dyn->d_val;
        }
    }
    if (!rela || !relasz || !relaent) {
        return;
    }
    exec_apply_rela(bias, rela, relasz, relaent);
}

/* Apply R_X86_64_RELATIVE entries from a RELA table (pure; unit-testable). */
void exec_apply_rela(u64 bias, u64 rela_addr, u64 relasz, u64 relaent) {
    if (!rela_addr || !relaent) {
        return;
    }
    for (u64 off = 0; off < relasz; off += relaent) {
        const elf_rela_t* r = (const elf_rela_t*)(rela_addr + off);
        if ((r->r_info & 0xffffffff) == R_X86_64_RELATIVE) {
            *(u64*)(bias + r->r_offset) = bias + (u64)r->r_addend;
        }
    }
}

int exec_load_elf(const void* elf_data, size_t size, u64* entry_out) {
    if (!elf_data || !entry_out || elf_validate(elf_data, size) < 0) {
        return -1;
    }

    const elf_header_t* ehdr = (const elf_header_t*)elf_data;
    const elf_phdr_t* phdr = (const elf_phdr_t*)((const u8*)elf_data + ehdr->e_phoff);

    /* Static-PIE (ET_DYN) is position-independent: relocate it to USER_LOAD_ADDR.
     * A fixed ET_EXEC keeps its own addresses (bias 0). */
    u64 bias = (ehdr->e_type == ET_DYN) ? USER_LOAD_ADDR : 0;

    for (u16 i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        u64 vaddr = phdr[i].p_vaddr + bias;
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

        /* Describe the segment as a VMA so faults inside it are understood. */
        process_t* proc = process_current();
        if (proc && proc->mm) {
            u64 vm = VM_READ;
            if (flags & 0x2) vm |= VM_WRITE;
            if (flags & 0x1) vm |= VM_EXEC;
            mm_insert_vma(proc->mm, page_start, page_end, vm);
        }
    }

    /* Relocate if position-independent (writable pages are mapped now). */
    if (bias) {
        exec_apply_relocations(ehdr, elf_data, bias);
    }

    /* Compute AT_PHDR: the in-memory address of the program headers. */
    u64 at_phdr = 0;
    for (u16 i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_PHDR) {
            at_phdr = phdr[i].p_vaddr + bias;
            break;
        }
    }
    if (!at_phdr) {
        for (u16 i = 0; i < ehdr->e_phnum; i++) {
            if (phdr[i].p_type == PT_LOAD && phdr[i].p_offset == 0) {
                at_phdr = phdr[i].p_vaddr + bias + ehdr->e_phoff;
                break;
            }
        }
    }

    g_last_elf_info.entry = ehdr->e_entry + bias;
    g_last_elf_info.phdr = at_phdr;
    g_last_elf_info.phent = ehdr->e_phentsize;
    g_last_elf_info.phnum = ehdr->e_phnum;
    g_last_elf_info.base = bias;

    *entry_out = g_last_elf_info.entry;
    return 0;
}

static u8 exec_file_buf[256 * 1024];

/* True if the blob came from the VFS (vs an embedded incbin blob). */
bool exec_blob_is_vfs(const void* blob) {
    return blob == exec_file_buf;
}

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

/* Map the user stack region as USER pages (grows down from USER_STACK_TOP). */
static int exec_map_user_stack(void) {
    u64 top = USER_STACK_TOP;
    u64 bottom = USER_STACK_TOP - USER_STACK_SIZE;
    for (u64 pa = bottom; pa < top; pa += PAGE_SIZE) {
        void* phys = pmm_alloc(1);
        if (!phys) {
            return -1;
        }
        memset(phys, 0, PAGE_SIZE);
        if (!vmm_map_page((void*)pa, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)) {
            return -1;
        }
    }

    /* Stack VMA marked GROWSDOWN so a fault just below `bottom` grows it. */
    process_t* proc = process_current();
    if (proc && proc->mm) {
        mm_insert_vma(proc->mm, bottom, top,
                      VM_READ | VM_WRITE | VM_ANON | VM_GROWSDOWN);
    }
    return 0;
}

/* AT_* auxiliary vector tags (subset; values match Linux). */
#define AT_NULL   0
#define AT_PHDR   3
#define AT_PHENT  4
#define AT_PHNUM  5
#define AT_PAGESZ 6
#define AT_BASE   7
#define AT_ENTRY  9
#define AT_UID    11
#define AT_EUID   12
#define AT_GID    13
#define AT_EGID   14
#define AT_HWCAP  16
#define AT_RANDOM 25
#define AT_SYSINFO_EHDR 33

/*
 * Lay out the initial user stack per the SysV AMD64 ABI:
 *
 *   high | argv/envp string data                       |
 *        | auxv[]  (AT_PAGESZ, AT_NULL)                 |
 *        | NULL                       (end of envp)     |
 *        | envp[]                                       |
 *        | NULL                       (end of argv)     |
 *        | argv[]                                       |
 *   rsp->| argc                                         |
 *
 * rsp is 16-byte aligned on entry with argc at [rsp]. Returns that user rsp.
 * `argv` must be kernel-resident (the old address space may already be gone).
 */
static u64 exec_setup_stack(char* const argv[]) {
    u64 sp = USER_STACK_TOP;

    int argc = 0;
    u64 argv_ptr[32];
    if (argv) {
        while (argv[argc] && argc < 31) {
            argc++;
        }
    }

    /* Copy the argument strings to the top of the stack (high to low). */
    for (int i = argc - 1; i >= 0; i--) {
        size_t len = strlen(argv[i]) + 1;
        sp -= len;
        memcpy((void*)sp, argv[i], len);
        argv_ptr[i] = sp;
    }

    /* 16 bytes of AT_RANDOM data live just above the vector. */
    sp -= 16;
    u64 at_random = sp;
    *(u64*)(sp) = 0x0123456789abcdefULL;
    *(u64*)(sp + 8) = 0xfedcba9876543210ULL;

    /* Assemble the pointer vector in a scratch buffer, then place it 16-aligned. */
    u64 vec[96];
    int n = 0;
    vec[n++] = (u64)argc;
    for (int i = 0; i < argc; i++) {
        vec[n++] = argv_ptr[i];
    }
    vec[n++] = 0;             /* argv terminator */
    vec[n++] = 0;             /* envp terminator (no env yet) */

    /* Auxiliary vector (SysV AMD64 ABI): describes the loaded image. */
    vec[n++] = AT_PHDR;   vec[n++] = g_last_elf_info.phdr;
    vec[n++] = AT_PHENT;  vec[n++] = g_last_elf_info.phent;
    vec[n++] = AT_PHNUM;  vec[n++] = g_last_elf_info.phnum;
    vec[n++] = AT_BASE;   vec[n++] = g_last_elf_info.base;
    vec[n++] = AT_ENTRY;  vec[n++] = g_last_elf_info.entry;
    vec[n++] = AT_PAGESZ; vec[n++] = PAGE_SIZE;
    vec[n++] = AT_UID;    vec[n++] = 0;
    vec[n++] = AT_EUID;   vec[n++] = 0;
    vec[n++] = AT_GID;    vec[n++] = 0;
    vec[n++] = AT_EGID;   vec[n++] = 0;
    vec[n++] = AT_HWCAP;  vec[n++] = 0;
    vec[n++] = AT_RANDOM; vec[n++] = at_random;
    vec[n++] = AT_SYSINFO_EHDR; vec[n++] = vdso_ehdr_addr();
    vec[n++] = AT_NULL;   vec[n++] = 0;

    u64 dest = (sp - (u64)n * 8) & ~0xFULL;
    /* Keep argc at a 16-aligned rsp: total slots must be even. */
    if (((dest / 8) & 1) != 0) {
        dest -= 8;
    }
    memcpy((void*)dest, vec, (size_t)n * 8);
    return dest;
}

void exec_jump_user(u64 entry) {
    /* Legacy no-argv entry (kept for callers that only need a bare stack). */
    gdt_init_user_segments();

    if (exec_map_user_stack() < 0) {
        DEBUG_ERROR("failed to map user stack");
    }

    process_t* cur = process_current();
    if (cur && cur->stack_base) {
        u64 ktop = ((u64)cur->stack_base + cur->stack_size) & ~0xFULL;
        tss_set_rsp0(ktop);
    }

    exec_iretq_user(entry, USER_STACK_TOP - 16, 0x202);
}

/* Unmap every user VMA of the current process (exec discards the old image). */
static void exec_teardown_user(process_t* proc) {
    if (!proc || !proc->mm) {
        return;
    }
    while (proc->mm->vmas) {
        vma_t* v = proc->mm->vmas;
        mm_remove_range(proc->mm, v->start, v->end);
    }
    mm_destroy(proc->mm);
    proc->mm = NULL;
}

int exec_run_path_argv(const char* path, char* const argv[]) {
    size_t size = 0;
    const void* blob = exec_resolve_path(path, &size);
    if (!blob) {
        return -1;
    }

    process_t* proc = process_current();

    /* Binaries loaded from the filesystem use the Linux x86-64 syscall ABI;
     * the embedded sh/nettest blobs keep this kernel's internal numbering. */
    bool from_vfs = exec_blob_is_vfs(blob);
    if (proc) {
        proc->linux_abi = from_vfs;
    }

    /* Replace the address space: discard the inherited/previous user mappings
     * (a no-op on first exec) and start from an empty VMA map. */
    if (proc) {
        exec_teardown_user(proc);
        proc->mm = mm_create();
        /* Descriptors marked close-on-exec are dropped across exec. */
        files_on_exec(proc->files);
        /* Caught signals reset to default across exec (POSIX). */
        if (proc->signal_state) {
            extern void signal_state_free(void*);
            signal_state_free(proc->signal_state);
            proc->signal_state = NULL;
        }
    }

    u64 entry = 0;
    if (exec_load_elf(blob, size, &entry) < 0) {
        return -1;
    }

    gdt_init_user_segments();
    if (exec_map_user_stack() < 0) {
        DEBUG_ERROR("failed to map user stack");
        return -1;
    }

    vdso_map_user();

    /* Route ring-3 traps/syscalls onto this task's kernel stack. */
    if (proc && proc->stack_base) {
        u64 ktop = ((u64)proc->stack_base + proc->stack_size) & ~0xFULL;
        tss_set_rsp0(ktop);
    }

    u64 user_sp = exec_setup_stack(argv);

    printk("[exec] %s ring3 entry=0x%x argc=%d (%u bytes)\n",
           path, (unsigned)entry, argv ? 1 : 0, (unsigned)size);
    exec_iretq_user(entry, user_sp, 0x202);
    return 0;
}

int exec_run_path(const char* path) {
    return exec_run_path_argv(path, NULL);
}
