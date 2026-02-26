#ifndef DM_H
#define DM_H

#include "types.h"

/* Device mapper target types */
#define DM_TARGET_LINEAR    0
#define DM_TARGET_STRIPED   1
#define DM_TARGET_MIRROR   2
#define DM_TARGET_SNAPSHOT 3
#define DM_TARGET_THIN     4

/* Device mapper table entry */
typedef struct dm_table_entry {
    u64 start;
    u64 length;
    u32 target_type;
    void* target_data;
    struct dm_table_entry* next;
} dm_table_entry_t;

/* Device mapper device */
typedef struct dm_device {
    u32 major;
    u32 minor;
    char name[64];
    dm_table_entry_t* table;
    bool active;
    struct dm_device* next;
} dm_device_t;

/* Initialize device mapper */
int dm_init(void);

/* Create device mapper device */
int dm_create(const char* name, u32 major, u32 minor);

/* Remove device mapper device */
int dm_remove(const char* name);

/* Load table */
int dm_table_load(const char* name, dm_table_entry_t* table);

/* Suspend device */
int dm_suspend(const char* name);

/* Resume device */
int dm_resume(const char* name);

/* Get device info */
int dm_info(const char* name, dm_device_t* info);

#endif /* DM_H */
