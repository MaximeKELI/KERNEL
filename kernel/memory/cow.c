#include "memory.h"
#include "mm.h"
#include "process.h"
#include "stdio.h"

/*
 * Central page-fault entry. Prefer the faulting process's VMA-aware dispatcher
 * (demand-zero anon, stack growth, COW); fall back to the bare COW handler for
 * kernel threads / early boot that have no mm yet.
 */
int cow_handle_page_fault(u64 cr2, u64 error_code) {
    process_t* p = process_current();
    if (p && p->mm) {
        if (mm_fault(p->mm, cr2, error_code) == 0) {
            return 0;
        }
    }
    return vmm_cow_fault((void*)cr2, error_code);
}
