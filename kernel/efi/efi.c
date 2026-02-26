#include "efi.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"

static efi_system_table_t* efi_table = NULL;

void efi_init(void* efi_system_table) {
    efi_table = (efi_system_table_t*)efi_system_table;
    
    if (efi_table && efi_table->signature == 0x5453595320494249ULL) {
        DEBUG_INFO("EFI (Extensible Firmware Interface) initialized");
    } else {
        DEBUG_WARN("EFI system table not found or invalid");
    }
}

efi_system_table_t* efi_get_system_table(void) {
    return efi_table;
}

int efi_get_memory_map(void* buffer, size_t* size) {
    (void)buffer;
    (void)size;
    /* Would get EFI memory map */
    return 0;
}
