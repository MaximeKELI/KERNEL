#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "io.h"

typedef struct {
    u64 entries[512];
} page_table_t;

static page_table_t* kernel_pml4 = NULL;

#define USER_ADDR_MAX 0x00007FFFFFFFFFFFULL

static bool addr_is_user(u64 addr) {
    return addr <= USER_ADDR_MAX;
}

static void clone_table(page_table_t* dst, page_table_t* src, int level, u64 virt_base, bool cow) {
    for (u32 i = 0; i < 512; i++) {
        u64 e = src->entries[i];
        if (!(e & PAGE_PRESENT)) {
            continue;
        }
        u64 virt = virt_base | ((u64)i << (12 + level * 9));

        if (level == 0) {
            if (!addr_is_user(virt)) {
                dst->entries[i] = e;
                continue;
            }
            if (cow) {
                dst->entries[i] = (e & ~PAGE_WRITABLE) | PAGE_USER;
                src->entries[i] = (e & ~PAGE_WRITABLE) | PAGE_USER;
            } else {
                dst->entries[i] = e;
            }
        } else {
            page_table_t* child_src = (page_table_t*)(e & ~0xFFF);
            page_table_t* child_dst = (page_table_t*)pmm_alloc(1);
            if (!child_dst) {
                continue;
            }
            memset(child_dst, 0, PAGE_SIZE);
            clone_table(child_dst, child_src, level - 1, virt, cow);
            dst->entries[i] = (u64)child_dst | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        }
    }
}

void vmm_init_user_mm(void) {
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    kernel_pml4 = (page_table_t*)(cr3 & ~0xFFF);
}

u64 vmm_get_cr3(void) {
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void vmm_switch_mm(u64 cr3) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3 & ~0xFFF) : "memory");
}

u64 vmm_fork_clone(void) {
    if (!kernel_pml4) {
        vmm_init_user_mm();
    }
    page_table_t* new_pml4 = (page_table_t*)pmm_alloc(1);
    if (!new_pml4) {
        return 0;
    }
    memset(new_pml4, 0, PAGE_SIZE);

    for (u32 i = 256; i < 512; i++) {
        new_pml4->entries[i] = kernel_pml4->entries[i];
    }
    for (u32 i = 0; i < 256; i++) {
        u64 e = kernel_pml4->entries[i];
        if (!(e & PAGE_PRESENT)) {
            continue;
        }
        page_table_t* pdpt_src = (page_table_t*)(e & ~0xFFF);
        page_table_t* pdpt_dst = (page_table_t*)pmm_alloc(1);
        if (!pdpt_dst) {
            continue;
        }
        memset(pdpt_dst, 0, PAGE_SIZE);
        clone_table(pdpt_dst, pdpt_src, 2, (u64)i << 39, true);
        new_pml4->entries[i] = (u64)pdpt_dst | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }
    return (u64)new_pml4;
}

int vmm_cow_fault(void* virt, u64 error) {
    if (!(error & 2)) {
        return -1;
    }
    u64 addr = (u64)virt;
    if (!addr_is_user(addr)) {
        return -1;
    }

    u64* pte = NULL;
    {
        u64 cr3 = vmm_get_cr3();
        page_table_t* pml4 = (page_table_t*)(cr3 & ~0xFFF);
        u64 pml4_idx = (addr >> 39) & 0x1FF;
        u64 pdpt_idx = (addr >> 30) & 0x1FF;
        u64 pd_idx = (addr >> 21) & 0x1FF;
        u64 pt_idx = (addr >> 12) & 0x1FF;
        u64 e0 = pml4->entries[pml4_idx];
        if (!(e0 & PAGE_PRESENT)) {
            return -1;
        }
        page_table_t* pdpt = (page_table_t*)(e0 & ~0xFFF);
        u64 e1 = pdpt->entries[pdpt_idx];
        if (!(e1 & PAGE_PRESENT)) {
            return -1;
        }
        page_table_t* pd = (page_table_t*)(e1 & ~0xFFF);
        u64 e2 = pd->entries[pd_idx];
        if (!(e2 & PAGE_PRESENT)) {
            return -1;
        }
        page_table_t* pt = (page_table_t*)(e2 & ~0xFFF);
        pte = &pt->entries[pt_idx];
    }

    if (!pte || !(*pte & PAGE_PRESENT)) {
        return -1;
    }
    if (*pte & PAGE_WRITABLE) {
        return -1;
    }

    void* old_phys = (void*)(*pte & ~0xFFF);
    void* new_phys = pmm_alloc(1);
    if (!new_phys) {
        return -1;
    }
    memcpy(new_phys, old_phys, PAGE_SIZE);
    *pte = ((u64)new_phys & ~0xFFF) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return 0;
}
