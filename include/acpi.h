#ifndef ACPI_H
#define ACPI_H

#include "types.h"

/* ACPI table header */
typedef struct __packed {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} acpi_header_t;

/* ACPI FADT */
typedef struct {
    acpi_header_t header;
    u32 firmware_ctrl;
    u32 dsdt;
    u8 reserved;
    u8 preferred_pm_profile;
    u16 sci_int;
    u32 smi_cmd;
    u8 acpi_enable;
    u8 acpi_disable;
    /* ... more fields ... */
} acpi_fadt_t;

/* Initialize ACPI */
void acpi_init(void);

/* Find ACPI table */
void* acpi_find_table(const char* signature);

/* Enable ACPI */
int acpi_enable(void);

/* Power management */
int acpi_suspend(u32 state);
int acpi_resume(void);

/* Device enumeration */
int acpi_enumerate_devices(void);

/* Thermal management */
int acpi_get_temperature(u32* temp);

/* Battery status */
int acpi_get_battery_status(u8* percentage, bool* charging);

#endif /* ACPI_H */
