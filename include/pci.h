#ifndef PCI_H
#define PCI_H

#include "types.h"

/* PCI configuration space */
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

/* PCI device structure */
typedef struct pci_device {
    u16 vendor_id;
    u16 device_id;
    u8 class_code;
    u8 subclass;
    u8 prog_if;
    u8 bus;
    u8 device;
    u8 function;
    u32 bar[6];
    void* private_data;
    struct pci_device* next;
} pci_device_t;

/* Initialize PCI */
void pci_init(void);

/* Scan PCI bus */
int pci_scan_bus(void);

/* Find PCI device */
pci_device_t* pci_find_device(u16 vendor_id, u16 device_id);
pci_device_t* pci_find_class(u8 class_code, u8 subclass);

/* Read PCI config */
u32 pci_read_config(u8 bus, u8 device, u8 function, u8 offset);
void pci_write_config(u8 bus, u8 device, u8 function, u8 offset, u32 value);

/* Register PCI driver */
typedef int (*pci_driver_init_t)(pci_device_t* dev);
void pci_register_driver(u16 vendor_id, u16 device_id, pci_driver_init_t init);

#endif /* PCI_H */
