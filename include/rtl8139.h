#ifndef RTL8139_H
#define RTL8139_H

#include "types.h"
#include "pci.h"
#include "ethernet.h"

#define RTL8139_VENDOR 0x10EC
#define RTL8139_DEVICE 0x8139

int rtl8139_probe(pci_device_t* pci, ethernet_device_t* eth);

#endif /* RTL8139_H */
