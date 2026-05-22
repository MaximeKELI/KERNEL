#include "stdio.h"
#include "pci.h"
#include "block.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "drivers/ata.h"

#define AHCI_CLASS     0x01
#define AHCI_SUBCLASS  0x06

static bool ahci_probed = false;

void ahci_init(void) {
    pci_device_t* dev = pci_find_class(AHCI_CLASS, AHCI_SUBCLASS);
    if (dev) {
        ahci_probed = true;
        register_blkdev(8, "ahci0", NULL);
        printk("[AHCI] controller found (using ATA PIO fallback)\n");
    } else {
        ata_init();
        printk("[AHCI] no controller; ATA primary PIO\n");
    }
}

bool ahci_present(void) {
    return ahci_probed;
}
