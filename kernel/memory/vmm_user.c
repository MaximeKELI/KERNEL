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

static void ensure_kernel_pml4(void) {
    if (!kernel_pml4) {
        u64 cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        kernel_pml4 = (page_table_t*)(cr3 & ~0xFFF);
    }
}

/*
 * Deep-copy one page-table level for fork().
 *   - Huge pages (PS) only ever describe the supervisor identity map here, so
 *     they are shared verbatim (never recursed into, never refcounted).
 *   - 4 KiB kernel leaves are shared verbatim.
 *   - 4 KiB user leaves are shared copy-on-write: both parent and child point at
 *     the same frame read-only, and the frame's refcount is bumped so it is only
 *     freed once every address space has dropped it.
 *   - Intermediate tables are duplicated so each address space owns its own.
 */
static void clone_table(page_table_t* dst, page_table_t* src, int level, u64 virt_base, bool cow) {
    for (u32 i = 0; i < 512; i++) {
        u64 e = src->entries[i];
        if (!(e & PAGE_PRESENT)) {
            continue;
        }
        u64 virt = virt_base | ((u64)i << (12 + level * 9));
        bool huge = (level > 0) && (e & PAGE_SIZE_2MB_FLAG);

        if (huge) {
            dst->entries[i] = e;               /* identity/kernel huge page: share */
            continue;
        }

        if (level == 0) {
            if (!addr_is_user(virt) || !(e & PAGE_USER)) {
                dst->entries[i] = e;           /* kernel leaf: share */
                continue;
            }
            if (cow) {
                u64 ro = e & ~PAGE_WRITABLE;    /* both sides read-only for COW */
                dst->entries[i] = ro;
                src->entries[i] = ro;
            } else {
                dst->entries[i] = e;
            }
            pmm_ref((void*)(e & ~0xFFFULL));    /* frame now has an extra sharer */
        } else {
            page_table_t* child_src = (page_table_t*)(e & ~0xFFFULL);
            page_table_t* child_dst = (page_table_t*)pmm_alloc(1);
            if (!child_dst) {
                continue;
            }
            memset(child_dst, 0, PAGE_SIZE);
            clone_table(child_dst, child_src, level - 1, virt, cow);
            dst->entries[i] = (u64)child_dst | (e & 0xFFFULL);
        }
    }
}

/* Free a privately-allocated page-table subtree, dropping refs on user leaves.
 * Shared kernel/identity leaves (supervisor, incl. huge pages) are never freed. */
static void free_table(page_table_t* t, int level) {
    for (u32 i = 0; i < 512; i++) {
        u64 e = t->entries[i];
        if (!(e & PAGE_PRESENT)) {
            continue;
        }
        bool huge = (level > 0) && (e & PAGE_SIZE_2MB_FLAG);
        if (huge) {
            continue;                          /* shared identity huge page */
        }
        if (level == 0) {
            if (e & PAGE_USER) {
                pmm_unref((void*)(e & ~0xFFFULL));
            }
        } else {
            free_table((page_table_t*)(e & ~0xFFFULL), level - 1);
        }
    }
    pmm_free(t, 1);
}

void vmm_init_user_mm(void) {
    ensure_kernel_pml4();
}

u64 vmm_get_cr3(void) {
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void vmm_switch_mm(u64 cr3) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3 & ~0xFFF) : "memory");
}

/*
 * Clone an address space from an explicit parent CR3 (COW). The kernel higher
 * half (PML4 256..511) is shared by pointer; the lower half — which mixes the
 * supervisor identity map and the user mappings in the same PML4[0] subtree — is
 * deep-copied so the child gets private page tables while user leaves are shared
 * copy-on-write. Returns the physical address of the new PML4, or 0 on failure.
 */
u64 vmm_fork_from(u64 parent_cr3) {
    ensure_kernel_pml4();
    page_table_t* parent = (page_table_t*)(parent_cr3 & ~0xFFFULL);
    page_table_t* new_pml4 = (page_table_t*)pmm_alloc(1);
    if (!new_pml4) {
        return 0;
    }
    memset(new_pml4, 0, PAGE_SIZE);

    for (u32 i = 256; i < 512; i++) {
        new_pml4->entries[i] = kernel_pml4->entries[i];   /* share kernel half */
    }
    for (u32 i = 0; i < 256; i++) {
        u64 e = parent->entries[i];
        if (!(e & PAGE_PRESENT)) {
            continue;
        }
        page_table_t* pdpt_src = (page_table_t*)(e & ~0xFFFULL);
        page_table_t* pdpt_dst = (page_table_t*)pmm_alloc(1);
        if (!pdpt_dst) {
            continue;
        }
        memset(pdpt_dst, 0, PAGE_SIZE);
        clone_table(pdpt_dst, pdpt_src, 2, (u64)i << 39, true);
        new_pml4->entries[i] = (u64)pdpt_dst | (e & 0xFFFULL);
    }
    return (u64)new_pml4;
}

u64 vmm_fork_clone(void) {
    return vmm_fork_from(vmm_get_cr3());
}

/*
 * Create a fresh address space that shares the entire current kernel mapping
 * (identity map included) but has no private user mappings of its own. Callers
 * add private user pages at PML4 slots that are empty in the kernel PML4 so they
 * never disturb shared subtrees.
 */
u64 vmm_create_user_space(void) {
    ensure_kernel_pml4();
    page_table_t* np = (page_table_t*)pmm_alloc(1);
    if (!np) {
        return 0;
    }
    for (u32 i = 0; i < 512; i++) {
        np->entries[i] = kernel_pml4->entries[i];
    }
    return (u64)np;
}

/*
 * Tear down an address space created by vmm_fork_from()/vmm_create_user_space().
 * Only PML4 entries that differ from the kernel PML4 are private to this space;
 * those subtrees are freed (dropping user-frame refs). Entries identical to the
 * kernel's are shared and left alone.
 */
void vmm_destroy_user_space(u64 cr3) {
    ensure_kernel_pml4();
    page_table_t* pml4 = (page_table_t*)(cr3 & ~0xFFFULL);
    if (!pml4 || pml4 == kernel_pml4) {
        return;
    }
    for (u32 i = 0; i < 256; i++) {
        u64 e = pml4->entries[i];
        if (!(e & PAGE_PRESENT)) {
            continue;
        }
        if (e == kernel_pml4->entries[i]) {
            continue;                          /* shared with the kernel space */
        }
        free_table((page_table_t*)(e & ~0xFFFULL), 2);
    }
    pmm_free(pml4, 1);
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

    /*
     * Fast path: this is the last sharer of the frame (refcount 1). No copy is
     * needed — just make it writable again. Matches Linux wp_page_reuse().
     */
    if (pmm_refcount(old_phys) <= 1) {
        *pte |= PAGE_WRITABLE;
        __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
        return 0;
    }

    void* new_phys = pmm_alloc(1);
    if (!new_phys) {
        return -1;
    }
    memcpy(new_phys, old_phys, PAGE_SIZE);
    *pte = ((u64)new_phys & ~0xFFF) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
    pmm_unref(old_phys);                       /* this space no longer shares it */
    return 0;
}
