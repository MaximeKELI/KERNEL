#ifndef HW_PORTS_H
#define HW_PORTS_H

#include "types.h"

/* -------------------------------------------------------------------------- */
/* Categories                                                                 */
/* -------------------------------------------------------------------------- */
typedef enum {
    HW_PORT_CAT_UNKNOWN = 0,
    HW_PORT_CAT_DMA,
    HW_PORT_CAT_TIMER,
    HW_PORT_CAT_INTERRUPT,
    HW_PORT_CAT_KEYBOARD,
    HW_PORT_CAT_MOUSE,
    HW_PORT_CAT_SERIAL,
    HW_PORT_CAT_PARALLEL,
    HW_PORT_CAT_STORAGE,
    HW_PORT_CAT_FLOPPY,
    HW_PORT_CAT_DISPLAY,
    HW_PORT_CAT_PCI,
    HW_PORT_CAT_CMOS,
    HW_PORT_CAT_SPEAKER,
    HW_PORT_CAT_MISC,
} hw_port_category_t;

/* -------------------------------------------------------------------------- */
/* DMA controller (i8237)                                                       */
/* -------------------------------------------------------------------------- */
#define DMA_ADDR_0          0x00
#define DMA_ADDR_1          0x02
#define DMA_ADDR_2          0x04
#define DMA_ADDR_3          0x06
#define DMA_COUNT_0         0x01
#define DMA_COUNT_1         0x03
#define DMA_COUNT_2         0x05
#define DMA_COUNT_3         0x07
#define DMA_STATUS          0x08
#define DMA_COMMAND         0x08
#define DMA_REQUEST         0x09
#define DMA_MASK_SINGLE     0x0A
#define DMA_MODE            0x0B
#define DMA_CLEAR_FLIPFLOP  0x0C
#define DMA_MASTER_CLEAR    0x0D
#define DMA_MASK_ALL        0x0F
#define DMA_PAGE_0          0x87
#define DMA_PAGE_1          0x83
#define DMA_PAGE_2          0x81
#define DMA_PAGE_3          0x82
#define DMA2_BASE           0xC0
#define DMA2_ADDR_4         0xC0
#define DMA2_ADDR_5         0xC4
#define DMA2_ADDR_6         0xC8
#define DMA2_ADDR_7         0xCC
#define DMA2_COUNT_4        0xC2
#define DMA2_COUNT_5        0xC6
#define DMA2_COUNT_6        0xCA
#define DMA2_COUNT_7        0xCE
#define DMA2_STATUS         0xD0
#define DMA2_COMMAND        0xD0
#define DMA2_MASK           0xD4
#define DMA2_MODE           0xD6
#define DMA2_CLEAR          0xD8
#define DMA2_MASTER_CLEAR   0xDA
#define DMA2_MASK_ALL       0xDE

/* -------------------------------------------------------------------------- */
/* PIT (8254)                                                                   */
/* -------------------------------------------------------------------------- */
#define PIT_CHANNEL0_DATA   0x40
#define PIT_CHANNEL1_DATA   0x41
#define PIT_CHANNEL2_DATA   0x42
#define PIT_MODE_COMMAND    0x43

/* -------------------------------------------------------------------------- */
/* PIC (8259)                                                                   */
/* -------------------------------------------------------------------------- */
#define PIC1_COMMAND        0x20
#define PIC1_DATA           0x21
#define PIC2_COMMAND        0xA0
#define PIC2_DATA           0xA1
#define PIC_EOI             0x20

/* -------------------------------------------------------------------------- */
/* Keyboard / controller                                                        */
/* -------------------------------------------------------------------------- */
#define KBD_DATA_PORT       0x60
#define KBD_STATUS_PORT     0x64
#define KBD_COMMAND_PORT    0x64
#define AUX_DATA_PORT       0x60
#define AUX_STATUS_PORT     0x64

/* -------------------------------------------------------------------------- */
/* CMOS / RTC                                                                   */
/* -------------------------------------------------------------------------- */
#define CMOS_INDEX          0x70
#define CMOS_DATA           0x71
#define CMOS_INDEX_NMI      0x72
#define CMOS_DATA_NMI       0x73

/* -------------------------------------------------------------------------- */
/* PC speaker                                                                   */
/* -------------------------------------------------------------------------- */
#define SPEAKER_PORT        0x61

/* -------------------------------------------------------------------------- */
/* Serial UART (16550 offsets from base)                                        */
/* -------------------------------------------------------------------------- */
#define COM1                0x3F8
#define COM2                0x2F8
#define COM3                0x3E8
#define COM4                0x2E8
#define UART_REG_RBR        0
#define UART_REG_THR        0
#define UART_REG_IER        1
#define UART_REG_IIR_FCR    2
#define UART_REG_LCR        3
#define UART_REG_MCR        4
#define UART_REG_LSR        5
#define UART_REG_MSR        6
#define UART_REG_SCR        7

/* -------------------------------------------------------------------------- */
/* Parallel (LPT)                                                               */
/* -------------------------------------------------------------------------- */
#define LPT1_DATA           0x378
#define LPT1_STATUS         0x379
#define LPT1_CONTROL        0x37A
#define LPT2_DATA           0x278
#define LPT2_STATUS         0x279
#define LPT2_CONTROL        0x27A
#define LPT3_DATA           0x3BC
#define LPT3_STATUS         0x3BD
#define LPT3_CONTROL        0x3BE

/* -------------------------------------------------------------------------- */
/* Floppy (FDC)                                                                 */
/* -------------------------------------------------------------------------- */
#define FDC_BASE            0x3F0
#define FDC_DIGITAL_OUTPUT  0x3F2
#define FDC_MAIN_STATUS     0x3F4
#define FDC_DATA_FIFO       0x3F5
#define FDC_DIR_CONTROL     0x3F7

/* -------------------------------------------------------------------------- */
/* ATA / IDE primary & secondary                                                */
/* -------------------------------------------------------------------------- */
#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_ERROR       0x1F1
#define ATA_PRIMARY_SECTOR      0x1F2
#define ATA_PRIMARY_LBA_LOW     0x1F3
#define ATA_PRIMARY_LBA_MID     0x1F4
#define ATA_PRIMARY_LBA_HIGH    0x1F5
#define ATA_PRIMARY_DEVICE      0x1F6
#define ATA_PRIMARY_COMMAND     0x1F7
#define ATA_PRIMARY_STATUS      0x1F7
#define ATA_PRIMARY_ALT_STATUS  0x3F6
#define ATA_SECONDARY_DATA      0x170
#define ATA_SECONDARY_ERROR     0x171
#define ATA_SECONDARY_SECTOR     0x172
#define ATA_SECONDARY_LBA_LOW   0x173
#define ATA_SECONDARY_LBA_MID   0x174
#define ATA_SECONDARY_LBA_HIGH  0x175
#define ATA_SECONDARY_DEVICE    0x176
#define ATA_SECONDARY_COMMAND   0x177
#define ATA_SECONDARY_STATUS    0x177
#define ATA_SECONDARY_ALT_STATUS 0x376

#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_STATUS_BSY          0x80
#define ATA_STATUS_DRDY         0x40
#define ATA_STATUS_DF           0x20
#define ATA_STATUS_DRQ          0x08
#define ATA_STATUS_ERR          0x01

/* -------------------------------------------------------------------------- */
/* VGA (CRT controller)                                                         */
/* -------------------------------------------------------------------------- */
#define VGA_CRTC_INDEX        0x3D4
#define VGA_CRTC_DATA         0x3D5
#define VGA_SEQ_INDEX         0x3C4
#define VGA_SEQ_DATA          0x3C5
#define VGA_GC_INDEX          0x3CE
#define VGA_GC_DATA           0x3CF
#define VGA_AC_INDEX          0x3C0
#define VGA_AC_WRITE          0x3C0
#define VGA_AC_READ           0x3C1
#define VGA_MISC_WRITE        0x3C2
#define VGA_MISC_READ         0x3CC
#define VGA_DAC_MASK          0x3C6
#define VGA_DAC_READ_INDEX    0x3C7
#define VGA_DAC_WRITE_INDEX   0x3C8
#define VGA_DAC_DATA          0x3C9
#define VGA_INPUT_STATUS_1    0x3DA

/* -------------------------------------------------------------------------- */
/* PCI configuration space                                                      */
/* -------------------------------------------------------------------------- */
#define PCI_CONFIG_ADDRESS    0xCF8
#define PCI_CONFIG_DATA       0xCFC

/* -------------------------------------------------------------------------- */
/* Legacy chipset / misc                                                        */
/* -------------------------------------------------------------------------- */
#define PORT_92_RESET         0x92
#define PORT_A20_GATE         0x92

void hw_ports_init(void);
const char* hw_port_category_name(hw_port_category_t cat);
const char* hw_port_lookup_name(u16 port);
hw_port_category_t hw_port_category(u16 port);
u32 hw_port_registry_count(void);
void hw_port_list_category(hw_port_category_t cat);

#endif /* HW_PORTS_H */
