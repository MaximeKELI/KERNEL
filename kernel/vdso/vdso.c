#include "vdso.h"
#include "memory.h"
#include "stdio.h"
#include "drivers/timer.h"
#include "exec.h"
#include "process.h"
#include "string.h"

/*
 * vDSO data page. The old value 0xFFFFC0000000 is NON-canonical (bits 63:48 do
 * not sign-extend bit 47), so mapping it and touching it #GP'd (invlpg / the
 * write below both fault on a non-canonical address). Place it in canonical user
 * space just below the ELF window (USER_LOAD_ADDR = 0x40000000) and clear of the
 * kernel image + heap (which end at ~12 MiB).
 */
#define VDSO_BASE 0x3FFFF000ULL
#define VDSO_SIZE PAGE_SIZE

typedef struct vdso_data {
    u64 ticks;
    u64 reserved[7];
} vdso_data_t;

static vdso_data_t* vdso_page = NULL;

void vdso_init(void) {
    void* phys = pmm_alloc(1);
    if (!phys) {
        return;
    }
    memset(phys, 0, PAGE_SIZE);
    vmm_map_page((void*)VDSO_BASE, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    vdso_page = (vdso_data_t*)VDSO_BASE;
    printk("[vdso] mapped at 0x%llx (clock_gettime data page)\n",
           (unsigned long long)VDSO_BASE);
}

void vdso_update(void) {
    if (vdso_page) {
        vdso_page->ticks = timer_get_ticks();
    }
}

u64 vdso_user_base(void) {
    return VDSO_BASE;
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
    memset(phys, 0, PAGE_SIZE);
    if (!vmm_map_page((void*)VDSO_BASE, phys, PAGE_PRESENT | PAGE_USER)) {
        pmm_free(phys, 1);
        vmm_switch_mm(old);
        return -1;
    }
    vdso_page = (vdso_data_t*)VDSO_BASE;
    vdso_page->ticks = timer_get_ticks();
    vmm_switch_mm(old);
    return 0;
}
