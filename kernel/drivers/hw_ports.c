#include "hw_ports.h"
#include "stdio.h"
#include "string.h"

typedef struct {
    u16 base;
    u16 size;
    const char* name;
    hw_port_category_t category;
} hw_port_entry_t;

static const hw_port_entry_t hw_port_table[] = {
    /* DMA */
    {DMA_ADDR_0, 16, "DMA1", HW_PORT_CAT_DMA},
    {DMA2_BASE, 32, "DMA2", HW_PORT_CAT_DMA},
    /* Timer */
    {PIT_CHANNEL0_DATA, 4, "PIT", HW_PORT_CAT_TIMER},
    /* PIC */
    {PIC1_COMMAND, 2, "PIC1", HW_PORT_CAT_INTERRUPT},
    {PIC2_COMMAND, 2, "PIC2", HW_PORT_CAT_INTERRUPT},
    /* Keyboard */
    {KBD_DATA_PORT, 1, "KBD_DATA", HW_PORT_CAT_KEYBOARD},
    {KBD_STATUS_PORT, 1, "KBD_CTRL", HW_PORT_CAT_KEYBOARD},
    /* CMOS */
    {CMOS_INDEX, 2, "CMOS", HW_PORT_CAT_CMOS},
    {CMOS_INDEX_NMI, 2, "CMOS_NMI", HW_PORT_CAT_CMOS},
    /* Speaker */
    {SPEAKER_PORT, 1, "SPEAKER", HW_PORT_CAT_SPEAKER},
    /* Serial */
    {COM1, 8, "COM1", HW_PORT_CAT_SERIAL},
    {COM2, 8, "COM2", HW_PORT_CAT_SERIAL},
    {COM3, 8, "COM3", HW_PORT_CAT_SERIAL},
    {COM4, 8, "COM4", HW_PORT_CAT_SERIAL},
    /* Parallel */
    {LPT1_DATA, 3, "LPT1", HW_PORT_CAT_PARALLEL},
    {LPT2_DATA, 3, "LPT2", HW_PORT_CAT_PARALLEL},
    {LPT3_DATA, 3, "LPT3", HW_PORT_CAT_PARALLEL},
    /* Storage */
    {ATA_PRIMARY_DATA, 8, "ATA_PRIMARY", HW_PORT_CAT_STORAGE},
    {ATA_PRIMARY_ALT_STATUS, 1, "ATA_PRI_CTRL", HW_PORT_CAT_STORAGE},
    {ATA_SECONDARY_DATA, 8, "ATA_SECONDARY", HW_PORT_CAT_STORAGE},
    {ATA_SECONDARY_ALT_STATUS, 1, "ATA_SEC_CTRL", HW_PORT_CAT_STORAGE},
    /* Floppy */
    {FDC_BASE, 8, "FDC", HW_PORT_CAT_FLOPPY},
    /* VGA */
    {VGA_AC_INDEX, 2, "VGA_ATTR", HW_PORT_CAT_DISPLAY},
    {VGA_SEQ_INDEX, 2, "VGA_SEQ", HW_PORT_CAT_DISPLAY},
    {VGA_GC_INDEX, 2, "VGA_GC", HW_PORT_CAT_DISPLAY},
    {VGA_CRTC_INDEX, 2, "VGA_CRTC", HW_PORT_CAT_DISPLAY},
    {VGA_DAC_MASK, 4, "VGA_DAC", HW_PORT_CAT_DISPLAY},
    {VGA_MISC_WRITE, 1, "VGA_MISC_W", HW_PORT_CAT_DISPLAY},
    {VGA_MISC_READ, 1, "VGA_MISC_R", HW_PORT_CAT_DISPLAY},
    {VGA_INPUT_STATUS_1, 1, "VGA_STATUS", HW_PORT_CAT_DISPLAY},
    /* PCI */
    {PCI_CONFIG_ADDRESS, 8, "PCI_CONFIG", HW_PORT_CAT_PCI},
    /* Misc */
    {PORT_92_RESET, 1, "SYS_CTRL_A20", HW_PORT_CAT_MISC},
    {0, 0, NULL, HW_PORT_CAT_UNKNOWN},
};

void hw_ports_init(void) {
    u32 n = 0;
    for (size_t i = 0; hw_port_table[i].name; i++) {
        n++;
    }
    printk("[HW] I/O port map: %u regions\n", n);
}

const char* hw_port_category_name(hw_port_category_t cat) {
    switch (cat) {
    case HW_PORT_CAT_DMA: return "DMA";
    case HW_PORT_CAT_TIMER: return "Timer";
    case HW_PORT_CAT_INTERRUPT: return "PIC";
    case HW_PORT_CAT_KEYBOARD: return "Keyboard";
    case HW_PORT_CAT_MOUSE: return "Mouse";
    case HW_PORT_CAT_SERIAL: return "Serial";
    case HW_PORT_CAT_PARALLEL: return "Parallel";
    case HW_PORT_CAT_STORAGE: return "Storage";
    case HW_PORT_CAT_FLOPPY: return "Floppy";
    case HW_PORT_CAT_DISPLAY: return "Display";
    case HW_PORT_CAT_PCI: return "PCI";
    case HW_PORT_CAT_CMOS: return "CMOS";
    case HW_PORT_CAT_SPEAKER: return "Speaker";
    case HW_PORT_CAT_MISC: return "Misc";
    default: return "?";
    }
}

static const hw_port_entry_t* hw_port_find_entry(u16 port) {
    for (size_t i = 0; hw_port_table[i].name; i++) {
        const hw_port_entry_t* e = &hw_port_table[i];
        if (port >= e->base && port < e->base + e->size) {
            return e;
        }
    }
    return NULL;
}

const char* hw_port_lookup_name(u16 port) {
    const hw_port_entry_t* e = hw_port_find_entry(port);
    return e ? e->name : NULL;
}

hw_port_category_t hw_port_category(u16 port) {
    const hw_port_entry_t* e = hw_port_find_entry(port);
    return e ? e->category : HW_PORT_CAT_UNKNOWN;
}

u32 hw_port_registry_count(void) {
    u32 n = 0;
    for (size_t i = 0; hw_port_table[i].name; i++) {
        n++;
    }
    return n;
}

void hw_port_list_category(hw_port_category_t cat) {
    for (size_t i = 0; hw_port_table[i].name; i++) {
        const hw_port_entry_t* e = &hw_port_table[i];
        if (cat != HW_PORT_CAT_UNKNOWN && e->category != cat) {
            continue;
        }
        printk("  0x%04X-0x%04X  %-14s  %s\n",
               e->base, e->base + e->size - 1, e->name,
               hw_port_category_name(e->category));
    }
}
