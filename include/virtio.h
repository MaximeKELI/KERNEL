#ifndef VIRTIO_H
#define VIRTIO_H

#include "types.h"
#include "pci.h"
#include "ethernet.h"

#define VIRTIO_VENDOR_ID        0x1AF4
#define VIRTIO_DEVICE_NET       0x1000
#define VIRTIO_DEVICE_NET_TRANS 0x1041

#define VIRTIO_PCI_HOST_FEATURES  0
#define VIRTIO_PCI_GUEST_FEATURES 4
#define VIRTIO_PCI_QUEUE_PFN      8
#define VIRTIO_PCI_QUEUE_NUM      12
#define VIRTIO_PCI_QUEUE_SEL      14
#define VIRTIO_PCI_QUEUE_NOTIFY   16
#define VIRTIO_PCI_STATUS         18
#define VIRTIO_PCI_ISR            19

#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER     2
#define VIRTIO_STATUS_DRIVER_OK  4
#define VIRTIO_STATUS_FAILED    128

#define VIRTIO_NET_F_MAC        (1 << 5)

#define VIRTIO_NET_QUEUE_RX     0
#define VIRTIO_NET_QUEUE_TX     1

int virtio_net_probe(pci_device_t* pci, ethernet_device_t* eth);

#endif /* VIRTIO_H */
