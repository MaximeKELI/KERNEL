#include "rtl8139.h"
#include "ethernet.h"
#include "net.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"

#define RTL_IDR0      0x00
#define RTL_TXSTATUS0 0x10
#define RTL_TXADDR0   0x20
#define RTL_CMD       0x37
#define RTL_RXBUF     0x30
#define RTL_RXEARLY   0xD0
#define RTL_CHIPCMD   0x37
#define RTL_IMR       0x3C
#define RTL_ISR       0x3E

#define RTL_CMD_RESET 0x10
#define RTL_CMD_RX_EN 0x08
#define RTL_CMD_TX_EN 0x04

#define RTL_RX_BUF_SIZE 8192
#define RTL_TX_BUF_SIZE 1536

typedef struct rtl8139_dev {
    u16 io_base;
    u8* rx_ring;
    u32 rx_ptr;
    u8* tx_bufs[4];
    ethernet_device_t* eth;
} rtl8139_dev_t;

static u8 rtl_in8(rtl8139_dev_t* r, u16 reg) {
    return inb(r->io_base + reg);
}

static void rtl_out8(rtl8139_dev_t* r, u16 reg, u8 val) {
    outb(r->io_base + reg, val);
}

static void rtl_out32(rtl8139_dev_t* r, u16 reg, u32 val) {
    outl(r->io_base + reg, val);
}

static int rtl8139_tx(ethernet_device_t* eth, void* data, size_t len) {
    rtl8139_dev_t* r = (rtl8139_dev_t*)eth->priv_data;
    if (!r || len < 14 || len > RTL_TX_BUF_SIZE) {
        return -1;
    }

    u8* buf = r->tx_bufs[0];
    memcpy(buf, data, len);
    rtl_out32(r, RTL_TXADDR0, (u32)(uintptr_t)buf);
    rtl_out32(r, RTL_TXSTATUS0, (u32)len);

    eth->tx_packets++;
    eth->tx_bytes += len;
    return 0;
}

static int rtl8139_rx(ethernet_device_t* eth, void* buffer, size_t buffer_size) {
    rtl8139_dev_t* r = (rtl8139_dev_t*)eth->priv_data;
    if (!r) {
        return -1;
    }

    u32 capr = (u32)rtl_in8(r, 0x38) | ((u32)rtl_in8(r, 0x39) << 8);
    u32 cbr = (u32)rtl_in8(r, 0x3A) | ((u32)rtl_in8(r, 0x3B) << 8);
    if (capr == cbr) {
        return 0;
    }

    u32 offset = capr % RTL_RX_BUF_SIZE;
    u16 status = *(u16*)(r->rx_ring + offset);
    u16 len = (*(u16*)(r->rx_ring + offset + 2)) - 4;
    if (!(status & 1) || len < 14 || len > buffer_size) {
        return 0;
    }

    offset = (offset + 4) % RTL_RX_BUF_SIZE;
    if (offset + len <= RTL_RX_BUF_SIZE) {
        memcpy(buffer, r->rx_ring + offset, len);
    } else {
        u32 first = RTL_RX_BUF_SIZE - offset;
        memcpy(buffer, r->rx_ring + offset, first);
        memcpy((u8*)buffer + first, r->rx_ring, len - first);
    }

    capr = (capr + len + 4 + 3) & ~3;
    capr %= RTL_RX_BUF_SIZE;
    rtl_out8(r, 0x38, (u8)(capr & 0xFF));
    rtl_out8(r, 0x39, (u8)((capr >> 8) & 0xFF));

    eth->rx_packets++;
    eth->rx_bytes += len;
    return (int)len;
}

static void rtl8139_poll(ethernet_device_t* eth) {
    u8 buf[1514];
    netif_t* iface = netif_list;
    while (iface && strcmp(iface->name, eth->name) != 0) {
        iface = iface->next;
    }
    if (!iface) {
        return;
    }
    for (;;) {
        int n = rtl8139_rx(eth, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        net_process_packet(iface, buf, (size_t)n);
    }
}

int rtl8139_probe(pci_device_t* pci, ethernet_device_t* eth) {
    if (!pci || !eth) {
        return -1;
    }

    pci_enable_device(pci);
    u16 io = (u16)(pci->bar[0] & ~0x3);
    if (!(pci->bar[0] & 1)) {
        return -1;
    }

    rtl8139_dev_t* r = (rtl8139_dev_t*)kzalloc(sizeof(rtl8139_dev_t));
    if (!r) {
        return -1;
    }

    r->io_base = io;
    r->eth = eth;
    r->rx_ring = (u8*)kzalloc(RTL_RX_BUF_SIZE + 16);
    r->tx_bufs[0] = (u8*)kzalloc(RTL_TX_BUF_SIZE);
    if (!r->rx_ring || !r->tx_bufs[0]) {
        kfree(r);
        return -1;
    }

    eth->priv_data = r;
    pci->private_data = r;

    rtl_out8(r, RTL_CHIPCMD, RTL_CMD_RESET);
    for (u32 i = 0; i < 100000; i++) {
        if (!(rtl_in8(r, RTL_CHIPCMD) & RTL_CMD_RESET)) {
            break;
        }
    }

    rtl_out32(r, RTL_RXBUF, (u32)(uintptr_t)r->rx_ring);
    rtl_out32(r, RTL_RXEARLY, 0);
    rtl_out8(r, RTL_CHIPCMD, RTL_CMD_RX_EN | RTL_CMD_TX_EN);

    for (int i = 0; i < 6; i++) {
        eth->mac_address[i] = rtl_in8(r, RTL_IDR0 + i);
    }

    eth->tx_fn = rtl8139_tx;
    eth->rx_fn = rtl8139_rx;
    eth->poll_fn = rtl8139_poll;

    printk("[rtl8139] %s up port 0x%x\n", eth->name, io);
    return 0;
}
