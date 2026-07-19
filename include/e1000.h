#ifndef E1000_H
#define E1000_H

#include "types.h"
#include "pci.h"
#include "ethernet.h"

/* Intel 82540EM Gigabit Ethernet — the NIC QEMU exposes by default. */
#define E1000_VENDOR 0x8086
#define E1000_DEVICE 0x100E

int e1000_probe(pci_device_t* pci, ethernet_device_t* eth);

#endif /* E1000_H */
