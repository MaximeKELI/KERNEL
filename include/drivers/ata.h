#ifndef ATA_H
#define ATA_H

#include "types.h"

/* Initialize ATA */
void ata_init(void);

/* Read sectors */
int ata_read_sectors(u64 lba, u32 count, void* buffer);

/* Write sectors */
int ata_write_sectors(u64 lba, u32 count, const void* buffer);

/* Get disk size */
u64 ata_get_disk_size(void);

#endif /* ATA_H */
