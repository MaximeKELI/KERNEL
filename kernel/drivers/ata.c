#include "drivers/ata.h"
#include "io.h"
#include "stdio.h"

#define ATA_PRIMARY_DATA       0x1F0
#define ATA_PRIMARY_ERROR      0x1F1
#define ATA_PRIMARY_SECTOR     0x1F2
#define ATA_PRIMARY_LBA_LOW    0x1F3
#define ATA_PRIMARY_LBA_MID    0x1F4
#define ATA_PRIMARY_LBA_HIGH   0x1F5
#define ATA_PRIMARY_DEVICE     0x1F6
#define ATA_PRIMARY_COMMAND    0x1F7
#define ATA_PRIMARY_STATUS     0x1F7
#define ATA_PRIMARY_ALT_STATUS 0x3F6

#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30
#define ATA_CMD_IDENTIFY       0xEC

#define ATA_STATUS_BSY         0x80
#define ATA_STATUS_DRDY        0x40
#define ATA_STATUS_DF          0x20
#define ATA_STATUS_DRQ         0x08
#define ATA_STATUS_ERR         0x01

static void ata_wait_ready(void) {
    while (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ));
}

int ata_read_sectors(u64 lba, u32 count, void* buffer) {
    if (count == 0) return -1;
    
    ata_wait_ready();
    
    /* Select drive and LBA */
    outb(ATA_PRIMARY_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECTOR, count);
    outb(ATA_PRIMARY_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);
    
    u16* buf = (u16*)buffer;
    
    for (u32 i = 0; i < count; i++) {
        ata_wait_ready();
        ata_wait_drq();
        
        if (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERR) {
            return -1;
        }
        
        /* Read 256 words (512 bytes) */
        for (u32 j = 0; j < 256; j++) {
            *buf++ = inw(ATA_PRIMARY_DATA);
        }
    }
    
    return 0;
}

int ata_write_sectors(u64 lba, u32 count, const void* buffer) {
    if (count == 0) return -1;
    
    ata_wait_ready();
    
    /* Select drive and LBA */
    outb(ATA_PRIMARY_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECTOR, count);
    outb(ATA_PRIMARY_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    const u16* buf = (const u16*)buffer;
    
    for (u32 i = 0; i < count; i++) {
        ata_wait_ready();
        ata_wait_drq();
        
        if (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERR) {
            return -1;
        }
        
        /* Write 256 words (512 bytes) */
        for (u32 j = 0; j < 256; j++) {
            outw(ATA_PRIMARY_DATA, *buf++);
        }
        
        /* Flush cache */
        outb(ATA_PRIMARY_COMMAND, 0xE7);
        ata_wait_ready();
    }
    
    return 0;
}

u64 ata_get_disk_size(void) {
    /* Simplified: assume 1GB disk */
    return 1024 * 1024 * 1024;
}

void ata_init(void) {
    printk("ATA: Initialized (simplified)\n");
}
