#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "io.h"

/* Page table structures */
typedef struct {
    u64 entries[512];
} page_table_t;

static page_table_t* pml4 = NULL;

/* Get page table entry */
static u64* get_pte(void* virt) {
    u64 addr = (u64)virt;
    u64 pml4_idx = (addr >> 39) & 0x1FF;
    u64 pdpt_idx = (addr >> 30) & 0x1FF;
    u64 pd_idx = (addr >> 21) & 0x1FF;
    u64 pt_idx = (addr >> 12) & 0x1FF;
    
    if (!pml4) return NULL;
    
    u64 pml4_entry = pml4->entries[pml4_idx];
    if (!(pml4_entry & PAGE_PRESENT)) return NULL;
    
    page_table_t* pdpt = (page_table_t*)(pml4_entry & ~0xFFF);
    u64 pdpt_entry = pdpt->entries[pdpt_idx];
    if (!(pdpt_entry & PAGE_PRESENT)) return NULL;
    
    page_table_t* pd = (page_table_t*)(pdpt_entry & ~0xFFF);
    u64 pd_entry = pd->entries[pd_idx];
    if (!(pd_entry & PAGE_PRESENT)) return NULL;
    /* 2 MiB huge page: there is no 4 KiB page table to index into. */
    if (pd_entry & PAGE_SIZE_2MB_FLAG) return NULL;
    
    page_table_t* pt = (page_table_t*)(pd_entry & ~0xFFF);
    return &pt->entries[pt_idx];
}

void vmm_init(void) {
    /* Get current CR3 */
    u64 cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    pml4 = (page_table_t*)(cr3 & ~0xFFF);
    
    printk("VMM: Initialized, PML4 at 0x%p\n", pml4);
}

void* vmm_map_page(void* virt, void* phys, u64 flags) {
    u64 addr = (u64)virt;
    u64 pml4_idx = (addr >> 39) & 0x1FF;
    u64 pdpt_idx = (addr >> 30) & 0x1FF;
    u64 pd_idx = (addr >> 21) & 0x1FF;
    u64 pt_idx = (addr >> 12) & 0x1FF;
    
    if (!pml4) return NULL;

    /*
     * When mapping a USER page, the USER bit must be set at EVERY level of the
     * walk (PML4E, PDPTE, PDE) — the CPU ANDs the U/S bits along the path, so a
     * supervisor intermediate entry would deny userspace regardless of the leaf.
     * The individual leaf PTE still gates real access, so making the upper
     * levels permissive is safe (this is exactly how Linux lays out its tables).
     */
    u64 user_bit = (flags & PAGE_USER) ? PAGE_USER : 0;

    /* Allocate/create page tables if needed */
    u64 pml4_entry = pml4->entries[pml4_idx];
    page_table_t* pdpt;
    
    if (!(pml4_entry & PAGE_PRESENT)) {
        /* Allocate new PDPT */
        pdpt = (page_table_t*)pmm_alloc(1);
        if (!pdpt) return NULL;
        memset(pdpt, 0, PAGE_SIZE);
        pml4->entries[pml4_idx] = (u64)pdpt | PAGE_PRESENT | PAGE_WRITABLE | user_bit;
    } else {
        pdpt = (page_table_t*)(pml4_entry & ~0xFFF);
        if (user_bit && !(pml4_entry & PAGE_USER)) {
            pml4->entries[pml4_idx] = pml4_entry | PAGE_USER;
        }
    }
    
    u64 pdpt_entry = pdpt->entries[pdpt_idx];
    page_table_t* pd;
    
    if (!(pdpt_entry & PAGE_PRESENT)) {
        pd = (page_table_t*)pmm_alloc(1);
        if (!pd) return NULL;
        memset(pd, 0, PAGE_SIZE);
        pdpt->entries[pdpt_idx] = (u64)pd | PAGE_PRESENT | PAGE_WRITABLE | user_bit;
    } else {
        pd = (page_table_t*)(pdpt_entry & ~0xFFF);
        if (user_bit && !(pdpt_entry & PAGE_USER)) {
            pdpt->entries[pdpt_idx] = pdpt_entry | PAGE_USER;
        }
    }
    
    u64 pd_entry = pd->entries[pd_idx];
    page_table_t* pt;
    
    if (!(pd_entry & PAGE_PRESENT)) {
        pt = (page_table_t*)pmm_alloc(1);
        if (!pt) return NULL;
        memset(pt, 0, PAGE_SIZE);
        pd->entries[pd_idx] = (u64)pt | PAGE_PRESENT | PAGE_WRITABLE | user_bit;
    } else if (pd_entry & PAGE_SIZE_2MB_FLAG) {
        if (!user_bit) {
            /* Region is already covered by the boot-time 2 MiB identity map;
             * a supervisor virt==phys mapping here is a no-op. */
            return virt;
        }
        /*
         * The user region falls inside a boot-time 2 MiB supervisor huge page.
         * Split it into 512 4 KiB PTEs that replicate the identity map, so we
         * can grant USER on individual pages while the rest stays supervisor.
         */
        pt = (page_table_t*)pmm_alloc(1);
        if (!pt) return NULL;
        u64 base = pd_entry & 0x000FFFFFFFE00000ULL;   /* 2 MiB-aligned physical base */
        u64 keep = pd_entry & (PAGE_WRITABLE | PAGE_PWT | PAGE_PCD | PAGE_GLOBAL);
        for (u64 i = 0; i < 512; i++) {
            pt->entries[i] = (base + (i << 12)) | PAGE_PRESENT | keep;
        }
        pd->entries[pd_idx] = (u64)pt | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    } else {
        pt = (page_table_t*)(pd_entry & ~0xFFF);
        if (user_bit && !(pd_entry & PAGE_USER)) {
            pd->entries[pd_idx] = pd_entry | PAGE_USER;
        }
    }
    
    /* Map page */
    pt->entries[pt_idx] = ((u64)phys & ~0xFFF) | flags | PAGE_PRESENT;
    
    /* Invalidate TLB (also drops any stale 2 MiB entry that covered this page) */
    __asm__ __volatile__("invlpg (%0)" : : "r"(virt) : "memory");
    
    return virt;
}

void vmm_unmap_page(void* virt) {
    u64* pte = get_pte(virt);
    if (pte && (*pte & PAGE_PRESENT)) {
        *pte = 0;
        __asm__ __volatile__("invlpg (%0)" : : "r"(virt) : "memory");
    }
}

void* vmm_alloc_pages(size_t pages) {
    /* Allocate physical pages */
    void* phys = pmm_alloc(pages);
    if (!phys) return NULL;
    
    /* Find virtual address (simplified: use physical address directly for kernel) */
    void* virt = phys;
    
    /* Map pages */
    for (size_t i = 0; i < pages; i++) {
        void* page_virt = (void*)((u64)virt + i * PAGE_SIZE);
        void* page_phys = (void*)((u64)phys + i * PAGE_SIZE);
        vmm_map_page(page_virt, page_phys, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    return virt;
}

void vmm_free_pages(void* virt, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        void* page_virt = (void*)((u64)virt + i * PAGE_SIZE);
        void* phys = vmm_get_phys(page_virt);
        if (phys) {
            vmm_unmap_page(page_virt);
            pmm_free(phys, 1);
        } else {
            /* Identity-mapped through a boot-time huge page: no 4 KiB PTE to
             * resolve, so release the physical block directly (phys == virt). */
            pmm_free(page_virt, 1);
        }
    }
}

bool vmm_is_mapped(void* virt) {
    u64* pte = get_pte(virt);
    return pte && (*pte & PAGE_PRESENT);
}

void* vmm_get_phys(void* virt) {
    u64* pte = get_pte(virt);
    if (pte && (*pte & PAGE_PRESENT)) {
        return (void*)(*pte & ~0xFFF);
    }
    return NULL;
}
