#include "mm.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

/* All VMAs describe user memory; a single fault may grow a stack this far. */
#define MM_MAX_STACK_GROW  (1UL * 1024 * 1024)
#define PAGE_MASK          (~(u64)(PAGE_SIZE - 1))

static inline u64 page_down(u64 a) { return a & PAGE_MASK; }
static inline u64 page_up(u64 a)   { return (a + PAGE_SIZE - 1) & PAGE_MASK; }

static u64 vma_page_flags(u64 vm_flags) {
    u64 f = PAGE_PRESENT | PAGE_USER;
    if (vm_flags & VM_WRITE) {
        f |= PAGE_WRITABLE;
    }
    return f;
}

mm_struct_t* mm_create(void) {
    mm_struct_t* mm = (mm_struct_t*)kzalloc(sizeof(mm_struct_t));
    if (!mm) {
        return NULL;
    }
    mm->vmas = NULL;
    mm->brk_start = USER_BRK_BASE;
    mm->brk = USER_BRK_BASE;
    mm->mmap_base = USER_MMAP_LOW;
    return mm;
}

mm_struct_t* mm_clone(mm_struct_t* src) {
    if (!src) {
        return NULL;
    }
    mm_struct_t* dst = mm_create();
    if (!dst) {
        return NULL;
    }
    dst->brk_start = src->brk_start;
    dst->brk = src->brk;
    dst->mmap_base = src->mmap_base;

    /* Copy the VMA list preserving order (append to tail). The physical pages
     * themselves are shared copy-on-write by vmm_fork_clone(); here we only
     * duplicate the address-space *description* so the child's faults resolve. */
    vma_t** tail = &dst->vmas;
    for (vma_t* v = src->vmas; v; v = v->next) {
        vma_t* c = (vma_t*)kzalloc(sizeof(vma_t));
        if (!c) {
            mm_destroy(dst);
            return NULL;
        }
        c->start = v->start;
        c->end = v->end;
        c->vm_flags = v->vm_flags;
        c->next = NULL;
        *tail = c;
        tail = &c->next;
    }
    return dst;
}

void mm_destroy(mm_struct_t* mm) {
    if (!mm) {
        return;
    }
    vma_t* v = mm->vmas;
    while (v) {
        vma_t* next = v->next;
        kfree(v);
        v = next;
    }
    kfree(mm);
}

vma_t* mm_find_vma(mm_struct_t* mm, u64 addr) {
    if (!mm) {
        return NULL;
    }
    for (vma_t* v = mm->vmas; v; v = v->next) {
        if (addr >= v->start && addr < v->end) {
            return v;
        }
        if (v->start > addr) {
            break;      /* list is sorted; no later VMA can contain addr */
        }
    }
    return NULL;
}

vma_t* mm_insert_vma(mm_struct_t* mm, u64 start, u64 end, u64 vm_flags) {
    if (!mm || end <= start) {
        return NULL;
    }
    vma_t* v = (vma_t*)kzalloc(sizeof(vma_t));
    if (!v) {
        return NULL;
    }
    v->start = start;
    v->end = end;
    v->vm_flags = vm_flags;

    /* Insert sorted by start. */
    vma_t** link = &mm->vmas;
    while (*link && (*link)->start < start) {
        link = &(*link)->next;
    }
    v->next = *link;
    *link = v;
    return v;
}

/* Drop the physical page backing `va` (if any) from the current address space. */
static void mm_unmap_one(u64 va) {
    void* phys = vmm_get_phys((void*)va);
    if (phys) {
        vmm_unmap_page((void*)va);
        pmm_unref(phys);
    }
}

int mm_remove_range(mm_struct_t* mm, u64 start, u64 end) {
    if (!mm || end <= start) {
        return -1;
    }
    start = page_down(start);
    end = page_up(end);

    /* Unmap the physical pages in the range. */
    for (u64 va = start; va < end; va += PAGE_SIZE) {
        mm_unmap_one(va);
    }

    /* Trim/split/remove the affected VMAs. */
    vma_t** link = &mm->vmas;
    while (*link) {
        vma_t* v = *link;
        if (v->end <= start || v->start >= end) {
            link = &v->next;                 /* no overlap */
            continue;
        }
        if (v->start >= start && v->end <= end) {
            *link = v->next;                 /* fully covered: drop it */
            kfree(v);
            continue;
        }
        if (v->start < start && v->end > end) {
            /* Split into [start,.. hole ..,end]: keep left, add right. */
            vma_t* right = (vma_t*)kzalloc(sizeof(vma_t));
            if (right) {
                right->start = end;
                right->end = v->end;
                right->vm_flags = v->vm_flags;
                right->next = v->next;
                v->next = right;
            }
            v->end = start;
            link = &v->next;
            continue;
        }
        if (v->start < start) {
            v->end = start;                  /* trim tail */
        } else {
            v->start = end;                  /* trim head */
        }
        link = &v->next;
    }
    return 0;
}

int mm_protect_range(mm_struct_t* mm, u64 start, u64 end, u64 vm_flags) {
    if (!mm || end <= start) {
        return -1;
    }
    start = page_down(start);
    end = page_up(end);

    /* The whole range must be mapped by VMAs. */
    for (u64 va = start; va < end; va += PAGE_SIZE) {
        if (!mm_find_vma(mm, va)) {
            return -1;
        }
    }

    for (vma_t* v = mm->vmas; v; v = v->next) {
        if (v->end <= start || v->start >= end) {
            continue;
        }
        /* Split so a VMA does not straddle the protected region's edges. */
        if (v->start < start && v->end > start) {
            vma_t* right = (vma_t*)kzalloc(sizeof(vma_t));
            if (right) {
                right->start = start;
                right->end = v->end;
                right->vm_flags = v->vm_flags;
                right->next = v->next;
                v->next = right;
                v->end = start;
            }
            v = right ? right : v;
        }
        if (v->start < end && v->end > end) {
            vma_t* right = (vma_t*)kzalloc(sizeof(vma_t));
            if (right) {
                right->start = end;
                right->end = v->end;
                right->vm_flags = v->vm_flags;
                right->next = v->next;
                v->next = right;
                v->end = end;
            }
        }
        v->vm_flags = vm_flags;

        /* Update any already-present PTEs in this VMA's covered range. */
        u64 lo = v->start > start ? v->start : start;
        u64 hi = v->end < end ? v->end : end;
        for (u64 va = lo; va < hi; va += PAGE_SIZE) {
            void* phys = vmm_get_phys((void*)va);
            if (phys) {
                vmm_map_page((void*)va, phys, vma_page_flags(vm_flags));
            }
        }
    }
    return 0;
}

int mm_fault(mm_struct_t* mm, u64 addr, u64 error_code) {
    if (!mm) {
        return -1;
    }
    u64 page = page_down(addr);
    bool present = (error_code & 0x1) != 0;
    bool write = (error_code & 0x2) != 0;

    if (present) {
        /* Protection violation on a mapped page: a write to a read-only page in
         * a writable VMA is a COW break. */
        vma_t* v = mm_find_vma(mm, addr);
        if (write && v && (v->vm_flags & VM_WRITE)) {
            return vmm_cow_fault((void*)addr, error_code);
        }
        return -1;
    }

    vma_t* v = mm_find_vma(mm, addr);
    if (!v) {
        /* Maybe a stack that needs to grow down into this page. */
        for (vma_t* s = mm->vmas; s; s = s->next) {
            if ((s->vm_flags & VM_GROWSDOWN) && addr < s->start &&
                addr >= s->start - MM_MAX_STACK_GROW) {
                s->start = page;
                v = s;
                break;
            }
        }
        if (!v) {
            return -1;      /* genuine SIGSEGV */
        }
    }

    if (write && !(v->vm_flags & VM_WRITE)) {
        return -1;          /* write to a read-only region */
    }

    /* Anonymous demand-zero: back the page with a fresh zeroed frame. */
    void* phys = pmm_alloc(1);
    if (!phys) {
        return -1;
    }
    memset(phys, 0, PAGE_SIZE);   /* identity-mapped low physical memory */
    if (!vmm_map_page((void*)page, phys, vma_page_flags(v->vm_flags))) {
        pmm_unref(phys);
        return -1;
    }
    return 0;
}

u64 mm_brk(mm_struct_t* mm, u64 new_brk) {
    if (!mm) {
        return (u64)-1;
    }
    if (new_brk == 0) {
        return mm->brk;                       /* query current break */
    }
    if (new_brk < mm->brk_start || new_brk > USER_BRK_MAX) {
        return mm->brk;                       /* refuse: keep old break */
    }

    u64 old = mm->brk;
    u64 old_end = page_up(old);
    u64 new_end = page_up(new_brk);

    if (new_brk > old) {
        /* Extend (or create) the heap VMA; pages are demand-paged. */
        vma_t* heap = mm_find_vma(mm, mm->brk_start);
        if (!heap) {
            if (!mm_insert_vma(mm, mm->brk_start, new_end,
                               VM_READ | VM_WRITE | VM_ANON)) {
                return old;
            }
        } else if (new_end > heap->end) {
            heap->end = new_end;
        }
    } else if (new_end < old_end) {
        /* Shrink: unmap the freed pages and trim the VMA. */
        mm_remove_range(mm, new_end, old_end);
        vma_t* heap = mm_find_vma(mm, mm->brk_start);
        if (heap) {
            heap->end = new_end > mm->brk_start ? new_end : mm->brk_start;
        }
    }

    mm->brk = new_brk;
    return mm->brk;
}

u64 mm_mmap(mm_struct_t* mm, u64 addr, u64 length, u64 vm_flags) {
    if (!mm || length == 0) {
        return (u64)-1;
    }
    u64 len = page_up(length);
    u64 start;

    if (addr) {
        start = page_down(addr);              /* fixed hint */
    } else {
        start = mm->mmap_base;
        if (start + len > USER_MMAP_HIGH) {
            return (u64)-1;
        }
        mm->mmap_base = start + len;
    }

    if (!mm_insert_vma(mm, start, start + len, vm_flags | VM_ANON)) {
        return (u64)-1;
    }
    return start;
}
