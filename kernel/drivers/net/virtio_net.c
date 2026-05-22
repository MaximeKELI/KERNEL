#include "virtio.h"
#include "ethernet.h"
#include "net.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"

#define VIRTQ_SIZE 16
#define VIRTQ_ALIGN 4096

typedef struct __packed {
    u64 addr;
    u32 len;
    u16 flags;
    u16 next;
} virtq_desc_t;

typedef struct __packed {
    u16 flags;
    u16 idx;
    u16 ring[VIRTQ_SIZE];
} virtq_avail_t;

typedef struct __packed {
    u16 flags;
    u16 idx;
    struct { u32 id; u32 len; } ring[VIRTQ_SIZE];
} virtq_used_t;

typedef struct {
    virtq_desc_t* desc;
    virtq_avail_t* avail;
    virtq_used_t* used;
    u16 last_used;
} virtqueue_t;

typedef struct virtio_net_dev {
    pci_device_t* pci;
    u16 port;
    u8 mac[6];
    virtqueue_t rxq;
    virtqueue_t txq;
    u8* rx_frames[VIRTQ_SIZE];
    u16 rx_frame_len[VIRTQ_SIZE];
    ethernet_device_t* eth;
} virtio_net_dev_t;

static virtio_net_dev_t* virtio_net;

static u16 vio_read16(u16 port, u16 off) {
    return inw(port + off);
}

static void vio_write16(u16 port, u16 off, u16 val) {
    outw(port + off, val);
}

static void vio_write32(u16 port, u16 off, u32 val) {
    outl(port + off, val);
}

static u8 vio_read8(u16 port, u16 off) {
    return inb(port + off);
}

static void vio_write8(u16 port, u16 off, u8 val) {
    outb(port + off, val);
}

static int virtqueue_init(virtqueue_t* vq, u16 port, u16 qsel) {
    vio_write16(port, VIRTIO_PCI_QUEUE_SEL, qsel);
    u16 num = vio_read16(port, VIRTIO_PCI_QUEUE_NUM);
    if (num == 0 || num > VIRTQ_SIZE) {
        num = VIRTQ_SIZE;
    }

    size_t desc_sz = sizeof(virtq_desc_t) * num;
    size_t avail_sz = sizeof(virtq_avail_t) + sizeof(u16) * num;
    size_t used_sz = sizeof(virtq_used_t) + sizeof(u32) * 2 * num;
    size_t total = desc_sz + avail_sz + used_sz + VIRTQ_ALIGN * 2;

    u8* mem = (u8*)kzalloc(total);
    if (!mem) {
        return -1;
    }

    uintptr_t base = ((uintptr_t)mem + VIRTQ_ALIGN - 1) & ~(uintptr_t)(VIRTQ_ALIGN - 1);
    vq->desc = (virtq_desc_t*)base;
    vq->avail = (virtq_avail_t*)(base + desc_sz);
    vq->used = (virtq_used_t*)(base + desc_sz + avail_sz);
    vq->last_used = 0;

    vio_write16(port, VIRTIO_PCI_QUEUE_NUM, num);
    vio_write32(port, VIRTIO_PCI_QUEUE_PFN, (u32)(base >> 12));
    return (int)num;
}

static int virtio_net_tx(ethernet_device_t* eth, void* data, size_t len) {
    virtio_net_dev_t* vn = (virtio_net_dev_t*)eth->priv_data;
    if (!vn || len > 1514 || len < 14) {
        return -1;
    }

    virtqueue_t* tx = &vn->txq;
    u16 idx = tx->avail->idx % VIRTQ_SIZE;

    u8* frame = (u8*)kzalloc(len);
    if (!frame) {
        return -1;
    }
    memcpy(frame, data, len);

    tx->desc[idx].addr = (u64)(uintptr_t)frame;
    tx->desc[idx].len = (u32)len;
    tx->desc[idx].flags = 0;
    tx->desc[idx].next = 0;
    tx->avail->ring[idx] = idx;
    tx->avail->idx++;

    vio_write16(vn->port, VIRTIO_PCI_QUEUE_SEL, VIRTIO_NET_QUEUE_TX);
    vio_write16(vn->port, VIRTIO_PCI_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

    eth->tx_packets++;
    eth->tx_bytes += len;
    return 0;
}

static int virtio_net_rx(ethernet_device_t* eth, void* buffer, size_t buffer_size) {
    virtio_net_dev_t* vn = (virtio_net_dev_t*)eth->priv_data;
    if (!vn) {
        return -1;
    }

    virtqueue_t* rx = &vn->rxq;
    if (rx->used->idx == rx->last_used) {
        return 0;
    }

    u16 used_idx = rx->last_used % VIRTQ_SIZE;
    u32 id = rx->used->ring[used_idx].id;
    u32 len = rx->used->ring[used_idx].len;
    rx->last_used++;

    if (id >= VIRTQ_SIZE || len < 14 || len > buffer_size) {
        return 0;
    }

    memcpy(buffer, vn->rx_frames[id], len);
    eth->rx_packets++;
    eth->rx_bytes += len;

    rx->avail->ring[rx->avail->idx % VIRTQ_SIZE] = id;
    rx->avail->idx++;
    vio_write16(vn->port, VIRTIO_PCI_QUEUE_SEL, VIRTIO_NET_QUEUE_RX);
    vio_write16(vn->port, VIRTIO_PCI_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);

    return (int)len;
}

static void virtio_net_poll(ethernet_device_t* eth) {
    u8 buf[1514];
    netif_t* iface = netif_list;
    while (iface && strcmp(iface->name, eth->name) != 0) {
        iface = iface->next;
    }
    if (!iface) {
        return;
    }

    for (;;) {
        int n = virtio_net_rx(eth, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        net_process_packet(iface, buf, (size_t)n);
    }
}

int virtio_net_probe(pci_device_t* pci, ethernet_device_t* eth) {
    if (!pci || !eth) {
        return -1;
    }

    pci_enable_device(pci);
    u16 port = (u16)(pci->bar[0] & ~0x3);
    if (!(pci->bar[0] & 1)) {
        printk("[virtio-net] BAR0 not I/O ports\n");
        return -1;
    }

    virtio_net_dev_t* vn = (virtio_net_dev_t*)kzalloc(sizeof(virtio_net_dev_t));
    if (!vn) {
        return -1;
    }

    vn->pci = pci;
    vn->port = port;
    vn->eth = eth;
    pci->private_data = vn;
    eth->priv_data = vn;

    vio_write8(port, VIRTIO_PCI_STATUS, 0);
    vio_write8(port, VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACK);
    vio_write8(port, VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER);

    u32 guest_features = VIRTIO_NET_F_MAC;
    vio_write32(port, VIRTIO_PCI_GUEST_FEATURES, guest_features);

    if (virtqueue_init(&vn->rxq, port, VIRTIO_NET_QUEUE_RX) < 0) {
        kfree(vn);
        return -1;
    }
    if (virtqueue_init(&vn->txq, port, VIRTIO_NET_QUEUE_TX) < 0) {
        kfree(vn);
        return -1;
    }

    for (u16 i = 0; i < VIRTQ_SIZE; i++) {
        vn->rx_frames[i] = (u8*)kzalloc(1514);
        if (!vn->rx_frames[i]) {
            return -1;
        }
        vn->rxq.desc[i].addr = (u64)(uintptr_t)vn->rx_frames[i];
        vn->rxq.desc[i].len = 1514;
        vn->rxq.desc[i].flags = 2;
        vn->rxq.desc[i].next = 0;
        vn->rxq.avail->ring[i] = i;
    }
    vn->rxq.avail->idx = VIRTQ_SIZE;
    vio_write16(port, VIRTIO_PCI_QUEUE_SEL, VIRTIO_NET_QUEUE_RX);
    vio_write16(port, VIRTIO_PCI_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);

    vn->mac[0] = 0x52;
    vn->mac[1] = 0x54;
    vn->mac[2] = 0x00;
    vn->mac[3] = 0x12;
    vn->mac[4] = 0x34;
    vn->mac[5] = (u8)(0x50 + eth->id);
    memcpy(eth->mac_address, vn->mac, 6);

    vio_write8(port, VIRTIO_PCI_STATUS,
               VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);

    eth->tx_fn = virtio_net_tx;
    eth->rx_fn = virtio_net_rx;
    eth->poll_fn = virtio_net_poll;
    virtio_net = vn;

    printk("[virtio-net] %s up port 0x%x MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->name, port,
           vn->mac[0], vn->mac[1], vn->mac[2], vn->mac[3], vn->mac[4], vn->mac[5]);
    return 0;
}
