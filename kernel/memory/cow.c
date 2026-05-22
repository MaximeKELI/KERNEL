#include "memory.h"
#include "stdio.h"

int cow_handle_page_fault(u64 cr2, u64 error_code) {
    return vmm_cow_fault((void*)cr2, error_code);
}
