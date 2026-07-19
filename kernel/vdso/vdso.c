#include "vdso.h"
#include "memory.h"
#include "stdio.h"
#include "drivers/timer.h"
#include "exec.h"
#include "process.h"
#include "string.h"
#include "abi/linux_syscall.h"

/*
 * vDSO page. Placed in canonical user space just below the ELF window
 * (USER_LOAD_ADDR = 0x40000000) and clear of the kernel image + heap.
 *
 * Real Linux exports a full DSO here with a symbol table so libc can resolve
 * __vdso_clock_gettime. We build a genuine executable code page (ELF magic at
 * offset 0 so AT_SYSINFO_EHDR points at a parseable header, plus the
 * __vdso_clock_gettime stub), and keep a data slot for the tick counter. Full
 * DSO symbol-table parsing by musl is deferred to the busybox bring-up (P8);
 * the stub already provides the fast-path→syscall fallback semantics.
 */
#define VDSO_BASE     0x3FFFF000ULL
#define VDSO_SIZE     PAGE_SIZE
#define VDSO_CODE_OFF 0x100
#define VDSO_DATA_OFF 0x800

static u64* vdso_ticks = NULL;

/* Write the ELF ident + a __vdso_clock_gettime code stub into a fresh page. */
static void vdso_fill(u8* page) {
    memset(page, 0, PAGE_SIZE);

    /* Minimal ELF64 identification so AT_SYSINFO_EHDR points at "ELF". */
    page[0] = 0x7F; page[1] = 'E'; page[2] = 'L'; page[3] = 'F';
    page[4] = 2;    /* ELFCLASS64 */
    page[5] = 1;    /* ELFDATA2LSB */
    page[6] = 1;    /* EV_CURRENT */
    /* e_type = ET_DYN (3) at offset 16, e_machine = 0x3E at offset 18. */
    page[16] = 3;   page[17] = 0;
    page[18] = 0x3E; page[19] = 0;

    /*
     * __vdso_clock_gettime(clockid, timespec*): fall back to the kernel via
     * SYSCALL. Bytes:
     *   b8 e4 00 00 00    mov eax, 228 (__NR_clock_gettime)
     *   0f 05             syscall
     *   c3                ret
     */
    u8* code = page + VDSO_CODE_OFF;
    code[0] = 0xB8;
    code[1] = (u8)(__NR_clock_gettime & 0xff);
    code[2] = (u8)((__NR_clock_gettime >> 8) & 0xff);
    code[3] = 0x00; code[4] = 0x00;
    code[5] = 0x0F; code[6] = 0x05;
    code[7] = 0xC3;
}

void vdso_init(void) {
    void* phys = pmm_alloc(1);
    if (!phys) {
        return;
    }
    vdso_fill((u8*)phys);
    /* Map executable+readable for user; kernel updates the tick slot in place. */
    vmm_map_page((void*)VDSO_BASE, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    vdso_ticks = (u64*)(VDSO_BASE + VDSO_DATA_OFF);
    *vdso_ticks = timer_get_ticks();
    printk("[vdso] code page at 0x%llx (__vdso_clock_gettime)\n",
           (unsigned long long)VDSO_BASE);
}

void vdso_update(void) {
    if (vdso_ticks) {
        *vdso_ticks = timer_get_ticks();
    }
}

u64 vdso_user_base(void) {
    return VDSO_BASE;
}

u64 vdso_ehdr_addr(void) {
    return VDSO_BASE;
}

/* Verify the vDSO page is a real ELF-magic'd code page with the syscall stub. */
int vdso_selftest(void) {
    u8* page = (u8*)kmalloc(PAGE_SIZE);
    if (!page) {
        return -1;
    }
    vdso_fill(page);
    int ok = (page[0] == 0x7F && page[1] == 'E' && page[2] == 'L' && page[3] == 'F' &&
              page[4] == 2 && page[16] == 3 /* ET_DYN */ && page[18] == 0x3E /* x86_64 */ &&
              page[VDSO_CODE_OFF] == 0xB8 &&
              page[VDSO_CODE_OFF + 5] == 0x0F && page[VDSO_CODE_OFF + 6] == 0x05 &&
              page[VDSO_CODE_OFF + 7] == 0xC3);
    kfree(page);
    return ok ? 0 : -1;
}

int vdso_map_user(void) {
    process_t* proc = process_current();
    if (!proc) {
        return -1;
    }
    if (proc->cr3 == 0) {
        proc->cr3 = vmm_get_cr3();
    }
    u64 old = vmm_get_cr3();
    vmm_switch_mm(proc->cr3);

    void* phys = pmm_alloc(1);
    if (!phys) {
        vmm_switch_mm(old);
        return -1;
    }
    vdso_fill((u8*)phys);
    if (!vmm_map_page((void*)VDSO_BASE, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)) {
        pmm_free(phys, 1);
        vmm_switch_mm(old);
        return -1;
    }
    vdso_ticks = (u64*)(VDSO_BASE + VDSO_DATA_OFF);
    *vdso_ticks = timer_get_ticks();
    vmm_switch_mm(old);
    return 0;
}
