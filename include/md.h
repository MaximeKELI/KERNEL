#ifndef MD_H
#define MD_H

#include "types.h"

/* MD RAID levels */
#define MD_RAID_LEVEL_LINEAR  0
#define MD_RAID_LEVEL_0       1
#define MD_RAID_LEVEL_1       2
#define MD_RAID_LEVEL_4       4
#define MD_RAID_LEVEL_5       5
#define MD_RAID_LEVEL_6       6

/* MD device structure */
typedef struct md_device {
    u32 major;
    u32 minor;
    char name[64];
    u32 raid_level;
    u32 num_disks;
    void* disks[16];
    bool active;
    struct md_device* next;
} md_device_t;

/* Initialize MD */
int md_init(void);

/* Create MD device */
int md_create(const char* name, u32 raid_level, void** disks, u32 num_disks);

/* Remove MD device */
int md_remove(const char* name);

/* Add disk to MD */
int md_add_disk(const char* name, void* disk);

/* Remove disk from MD */
int md_remove_disk(const char* name, void* disk);

/* Get MD status */
int md_status(const char* name, md_device_t* status);

#endif /* MD_H */
