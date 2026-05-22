#include "memory.h"
#include "stdio.h"
#include "drivers/timer.h"
#include "exec.h"

#define VDSO_BASE 0xFFFFC0000000ULL
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
