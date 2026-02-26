#ifndef EFI_H
#define EFI_H

#include "types.h"

/* EFI system table */
typedef struct {
    u64 signature;
    u32 revision;
    u32 header_size;
    u32 crc32;
    u32 reserved;
    u64 firmware_vendor;
    u32 firmware_revision;
    u64 con_in_handle;
    void* con_in;
    u64 con_out_handle;
    void* con_out;
    u64 stderr_handle;
    void* std_err;
    void* runtime_services;
    void* boot_services;
} efi_system_table_t;

/* Initialize EFI */
void efi_init(void* efi_system_table);

/* Get EFI system table */
efi_system_table_t* efi_get_system_table(void);

/* EFI memory map */
int efi_get_memory_map(void* buffer, size_t* size);

#endif /* EFI_H */
