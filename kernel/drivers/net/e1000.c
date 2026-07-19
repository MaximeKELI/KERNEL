#include "e1000.h"
#include "ethernet.h"
#include "net.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"

/* MMIO register offsets (Intel 82540EM / QEMU e1000). */
#define E1000_CTRL   0x0000
#define E1000_STATUS 0x0008
#define E1000_EERD   0x0014
#define E1000_ICR    0x00C0
#define E1000_IMS    0x00D0
#define E1000_IMC    0x00D8
#define E1000_RCTL   0x0100
#define E1000_TCTL   0x0400
#define E1000_TIPG   0x0410
#define E1000_RDBAL  0x2800
#define E1000_RDBAH  0x2804
#define E1000_RDLEN  0x2808
#define E1000_RDH    0x2810
#define E1000_RDT    0x2818
#define E1000_TDBAL  0x3800
#define E1000_TDBAH  0x3804
#define E1000_TDLEN  0x3808
#define E1000_TDH    0x3810
#define E1000_TDT    0x3818
#define E1000_MTA    0x5200
#define E1000_RAL0   0x5400
#define E1000_RAH0   0x5404

/* CTRL bits */
#define CTRL_SLU  0x00000040  /* set link up */
#define CTRL_ASDE 0x00000020  /* auto speed detect enable */
#define CTRL_RST  0x04000000  /* device reset */

/* RCTL bits */
#define RCTL_EN     0x00000002  /* receiver enable */
#define RCTL_UPE    0x00000008  /* unicast promiscuous */
#define RCTL_MPE    0x00000010  /* multicast promiscuous */
#define RCTL_BAM    0x00008000  /* broadcast accept */
#define RCTL_SECRC  0x04000000  /* strip Ethernet CRC */
/* BSIZE = 00 -> 2048 byte buffers */

/* TCTL bits */
#define TCTL_EN   0x00000002
#define TCTL_PSP  0x00000008  /* pad short packets */
#define TCTL_CT   (0x10 << 4)   /* collision threshold */
#define TCTL_COLD (0x40 << 12)  /* collision distance (full duplex) */

/* TX descriptor command / status */
#define TXD_CMD_EOP  0x01
#define TXD_CMD_IFCS 0x02
#define TXD_CMD_RS   0x08
#define TXD_STAT_DD  0x01

/* RX descriptor status */
#define RXD_STAT_DD  0x01
#define RXD_STAT_EOP 0x02

#define E1000_NUM_RX 32
#define E1000_NUM_TX 8
#define E1000_BUF_SIZE 2048

typedef struct {
    volatile u64 addr;
    volatile u16 length;
    volatile u16 checksum;
    volatile u8  status;
    volatile u8  errors;
    volatile u16 special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    volatile u64 addr;
    volatile u16 length;
    volatile u8  cso;
    volatile u8  cmd;
    volatile u8  status;
    volatile u8  css;
    volatile u16 special;
} __attribute__((packed)) e1000_tx_desc_t;

typedef struct e1000_dev {
    volatile u8*      mmio;
    e1000_rx_desc_t*  rx_ring;
    e1000_tx_desc_t*  tx_ring;
    u8*               rx_buf[E1000_NUM_RX];
    u8*               tx_buf[E1000_NUM_TX];
    u32               rx_cur;
    u32               tx_cur;
    ethernet_device_t* eth;
} e1000_dev_t;

static inline u32 e1000_read(e1000_dev_t* e, u32 reg) {
    return *(volatile u32*)(e->mmio + reg);
}

static inline void e1000_write(e1000_dev_t* e, u32 reg, u32 val) {
    *(volatile u32*)(e->mmio + reg) = val;
}

static void e1000_delay(void) {
    for (volatile u32 i = 0; i < 1000000; i++) {
        __asm__ __volatile__("pause");
    }
}

/* Align a heap allocation up to `align`; identity-mapped low memory means the
 * returned virtual address is also the physical (DMA) address. */
static void* e1000_alloc_aligned(size_t size, size_t align) {
    u8* p = (u8*)kzalloc(size + align);
    if (!p) {
        return NULL;
    }
    uintptr_t a = ((uintptr_t)p + (align - 1)) & ~(uintptr_t)(align - 1);
    return (void*)a;
}

static u16 e1000_eeprom_read(e1000_dev_t* e, u8 addr) {
    e1000_write(e, E1000_EERD, ((u32)addr << 8) | 0x1);
    u32 val = 0;
    for (u32 i = 0; i < 100000; i++) {
        val = e1000_read(e, E1000_EERD);
        if (val & (1 << 4)) {  /* DONE */
            break;
        }
    }
    return (u16)(val >> 16);
}

static void e1000_read_mac(e1000_dev_t* e, u8 mac[6]) {
    u32 ral = e1000_read(e, E1000_RAL0);
    u32 rah = e1000_read(e, E1000_RAH0);

    if (rah & 0x80000000) {  /* Address Valid: QEMU pre-loaded the MAC */
        mac[0] = (u8)(ral & 0xFF);
        mac[1] = (u8)((ral >> 8) & 0xFF);
        mac[2] = (u8)((ral >> 16) & 0xFF);
        mac[3] = (u8)((ral >> 24) & 0xFF);
        mac[4] = (u8)(rah & 0xFF);
        mac[5] = (u8)((rah >> 8) & 0xFF);
        return;
    }

    /* Fall back to EEPROM words 0..2. */
    for (int i = 0; i < 3; i++) {
        u16 w = e1000_eeprom_read(e, (u8)i);
        mac[i * 2] = (u8)(w & 0xFF);
        mac[i * 2 + 1] = (u8)(w >> 8);
    }
}

static void e1000_setup_rx(e1000_dev_t* e) {
    for (u32 i = 0; i < E1000_NUM_RX; i++) {
        e->rx_buf[i] = (u8*)e1000_alloc_aligned(E1000_BUF_SIZE, 16);
        e->rx_ring[i].addr = (u64)(uintptr_t)e->rx_buf[i];
        e->rx_ring[i].status = 0;
    }
    e->rx_cur = 0;

    e1000_write(e, E1000_RDBAL, (u32)(uintptr_t)e->rx_ring);
    e1000_write(e, E1000_RDBAH, 0);
    e1000_write(e, E1000_RDLEN, E1000_NUM_RX * (u32)sizeof(e1000_rx_desc_t));
    e1000_write(e, E1000_RDH, 0);
    e1000_write(e, E1000_RDT, E1000_NUM_RX - 1);

    e1000_write(e, E1000_RCTL,
                RCTL_EN | RCTL_BAM | RCTL_UPE | RCTL_MPE | RCTL_SECRC);
}

static void e1000_setup_tx(e1000_dev_t* e) {
    for (u32 i = 0; i < E1000_NUM_TX; i++) {
        e->tx_buf[i] = (u8*)e1000_alloc_aligned(E1000_BUF_SIZE, 16);
        e->tx_ring[i].addr = 0;
        e->tx_ring[i].status = TXD_STAT_DD;  /* mark free */
    }
    e->tx_cur = 0;

    e1000_write(e, E1000_TDBAL, (u32)(uintptr_t)e->tx_ring);
    e1000_write(e, E1000_TDBAH, 0);
    e1000_write(e, E1000_TDLEN, E1000_NUM_TX * (u32)sizeof(e1000_tx_desc_t));
    e1000_write(e, E1000_TDH, 0);
    e1000_write(e, E1000_TDT, 0);

    e1000_write(e, E1000_TCTL, TCTL_EN | TCTL_PSP | TCTL_CT | TCTL_COLD);
    e1000_write(e, E1000_TIPG, 0x0060200A);
}

static int e1000_tx(ethernet_device_t* eth, void* data, size_t len) {
    e1000_dev_t* e = (e1000_dev_t*)eth->priv_data;
    if (!e || len < 14) {
        return -1;
    }
    if (len > E1000_BUF_SIZE) {
        len = E1000_BUF_SIZE;
    }

    u32 cur = e->tx_cur;
    /* Wait for the descriptor to be free (previous TX done). */
    for (u32 i = 0; i < 1000000 && !(e->tx_ring[cur].status & TXD_STAT_DD); i++) {
        __asm__ __volatile__("pause");
    }

    memcpy(e->tx_buf[cur], data, len);
    e->tx_ring[cur].addr = (u64)(uintptr_t)e->tx_buf[cur];
    e->tx_ring[cur].length = (u16)len;
    e->tx_ring[cur].cmd = TXD_CMD_EOP | TXD_CMD_IFCS | TXD_CMD_RS;
    e->tx_ring[cur].status = 0;

    e->tx_cur = (cur + 1) % E1000_NUM_TX;
    e1000_write(e, E1000_TDT, e->tx_cur);

    eth->tx_packets++;
    eth->tx_bytes += len;
    return 0;
}

static int e1000_rx(ethernet_device_t* eth, void* buffer, size_t buffer_size) {
    e1000_dev_t* e = (e1000_dev_t*)eth->priv_data;
    if (!e) {
        return -1;
    }

    u32 cur = e->rx_cur;
    if (!(e->rx_ring[cur].status & RXD_STAT_DD)) {
        return 0;
    }

    u16 len = e->rx_ring[cur].length;
    if (len > buffer_size) {
        len = (u16)buffer_size;
    }
    memcpy(buffer, e->rx_buf[cur], len);

    e->rx_ring[cur].status = 0;
    e1000_write(e, E1000_RDT, cur);           /* hand descriptor back to HW */
    e->rx_cur = (cur + 1) % E1000_NUM_RX;

    eth->rx_packets++;
    eth->rx_bytes += len;
    return (int)len;
}

static void e1000_poll(ethernet_device_t* eth) {
    u8 buf[1514];
    netif_t* iface = netif_list;
    while (iface && strcmp(iface->name, eth->name) != 0) {
        iface = iface->next;
    }
    if (!iface) {
        return;
    }
    for (;;) {
        int n = e1000_rx(eth, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        net_process_packet(iface, buf, (size_t)n);
    }
}

int e1000_probe(pci_device_t* pci, ethernet_device_t* eth) {
    if (!pci || !eth) {
        return -1;
    }

    /* BAR0 must be a memory BAR (bit 0 clear). */
    if (pci->bar[0] & 0x1) {
        return -1;
    }

    pci_enable_device(pci);  /* IO + MEM + bus mastering */

    e1000_dev_t* e = (e1000_dev_t*)kzalloc(sizeof(e1000_dev_t));
    if (!e) {
        return -1;
    }

    e->mmio = (volatile u8*)(uintptr_t)(pci->bar[0] & ~0xFUL);
    e->eth = eth;
    e->rx_ring = (e1000_rx_desc_t*)e1000_alloc_aligned(
        E1000_NUM_RX * sizeof(e1000_rx_desc_t), 16);
    e->tx_ring = (e1000_tx_desc_t*)e1000_alloc_aligned(
        E1000_NUM_TX * sizeof(e1000_tx_desc_t), 16);
    if (!e->rx_ring || !e->tx_ring) {
        kfree(e);
        return -1;
    }

    /* Reset, then mask all interrupts (we poll). */
    e1000_write(e, E1000_CTRL, e1000_read(e, E1000_CTRL) | CTRL_RST);
    e1000_delay();
    e1000_write(e, E1000_IMC, 0xFFFFFFFF);
    (void)e1000_read(e, E1000_ICR);

    /* Link up + auto speed. */
    e1000_write(e, E1000_CTRL,
                e1000_read(e, E1000_CTRL) | CTRL_SLU | CTRL_ASDE);

    e1000_read_mac(e, eth->mac_address);

    /* Program receive address filter 0 with our MAC (AV bit set). */
    u32 ral = (u32)eth->mac_address[0] | ((u32)eth->mac_address[1] << 8) |
              ((u32)eth->mac_address[2] << 16) | ((u32)eth->mac_address[3] << 24);
    u32 rah = (u32)eth->mac_address[4] | ((u32)eth->mac_address[5] << 8) |
              0x80000000;
    e1000_write(e, E1000_RAL0, ral);
    e1000_write(e, E1000_RAH0, rah);

    /* Clear the multicast table. */
    for (u32 i = 0; i < 128; i++) {
        e1000_write(e, E1000_MTA + i * 4, 0);
    }

    eth->priv_data = e;
    pci->private_data = e;

    e1000_setup_rx(e);
    e1000_setup_tx(e);

    eth->tx_fn = e1000_tx;
    eth->rx_fn = e1000_rx;
    eth->poll_fn = e1000_poll;

    printk("[e1000] %s up mmio=0x%x mac %x:%x:%x:%x:%x:%x\n",
           eth->name, (unsigned)(uintptr_t)e->mmio,
           eth->mac_address[0], eth->mac_address[1], eth->mac_address[2],
           eth->mac_address[3], eth->mac_address[4], eth->mac_address[5]);
    return 0;
}
