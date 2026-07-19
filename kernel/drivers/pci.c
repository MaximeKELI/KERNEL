#include "pci.h"
#include "io.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static pci_device_t* pci_devices = NULL;
static spinlock_t pci_lock = SPINLOCK_INIT;

u32 pci_read_config(u8 bus, u8 device, u8 function, u8 offset) {
    u32 address = (u32)((1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(u8 bus, u8 device, u8 function, u8 offset, u32 value) {
    u32 address = (u32)((1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

void pci_init(void) {
    DEBUG_INFO("PCI subsystem initialized");
    pci_scan_bus();
}

int pci_scan_bus(void) {
    u32 devices_found = 0;

    /*
     * Scan bus 0 only. On the QEMU i440fx/q35 default the host bridge does not
     * decode buses 1..255, so config reads there alias back to bus 0 and every
     * slot looks populated -> thousands of phantom devices. All real devices
     * (including virtio-net) live on bus 0. (Devices behind PCI-PCI bridges
     * would need recursive scanning, which QEMU's default topology doesn't use.)
     */
    for (u8 bus = 0; bus < 1; bus++) {
        for (u8 device = 0; device < 32; device++) {
            for (u8 function = 0; function < 8; function++) {
                u32 vendor_id = pci_read_config(bus, device, function, 0) & 0xFFFF;
                
                /* 0xFFFF = no device; 0x0000 = host bridge not decoding this
                 * slot (returns all-zeros). Treating the latter as a device
                 * makes every empty slot look populated and floods the bus with
                 * thousands of bogus entries. Skip both. */
                if (vendor_id == 0xFFFF || vendor_id == 0x0000) continue;
                
                u32 device_id = (pci_read_config(bus, device, function, 0) >> 16) & 0xFFFF;
                u32 class_rev = pci_read_config(bus, device, function, 8);
                u8 class_code = (class_rev >> 24) & 0xFF;
                u8 subclass = (class_rev >> 16) & 0xFF;
                u8 prog_if = (class_rev >> 8) & 0xFF;
                
                /* Allocate device */
                pci_device_t* dev = (pci_device_t*)kmalloc(sizeof(pci_device_t));
                if (!dev) continue;
                
                dev->vendor_id = vendor_id;
                dev->device_id = device_id;
                dev->class_code = class_code;
                dev->subclass = subclass;
                dev->prog_if = prog_if;
                dev->bus = bus;
                dev->device = device;
                dev->function = function;
                
                /* Read BARs */
                for (int i = 0; i < 6; i++) {
                    dev->bar[i] = pci_read_config(bus, device, function, 0x10 + i * 4);
                }
                
                spinlock_lock(&pci_lock);
                dev->next = pci_devices;
                pci_devices = dev;
                spinlock_unlock(&pci_lock);
                
                devices_found++;
                
                DEBUG_INFO("PCI device: %x:%x class %x:%x at %x:%x.%x",
                          vendor_id, device_id, class_code, subclass,
                          bus, device, function);

                /* Safety cap: never let a misbehaving host bridge run the list
                 * (and the heap) away. */
                if (devices_found >= 256) {
                    DEBUG_INFO("PCI scan capped at %u devices", devices_found);
                    return devices_found;
                }
                
                /* Only check function 0 for multi-function devices */
                if (function == 0 && !(pci_read_config(bus, device, 0, 0x0E) & 0x80)) {
                    break;
                }
            }
        }
    }
    
    DEBUG_INFO("PCI scan complete: %u devices found", devices_found);
    return devices_found;
}

pci_device_t* pci_find_device(u16 vendor_id, u16 device_id) {
    spinlock_lock(&pci_lock);
    
    pci_device_t* dev = pci_devices;
    while (dev) {
        if (dev->vendor_id == vendor_id && dev->device_id == device_id) {
            spinlock_unlock(&pci_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&pci_lock);
    return NULL;
}

pci_device_t* pci_find_class(u8 class_code, u8 subclass) {
    spinlock_lock(&pci_lock);
    
    pci_device_t* dev = pci_devices;
    while (dev) {
        if (dev->class_code == class_code && dev->subclass == subclass) {
            spinlock_unlock(&pci_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&pci_lock);
    return NULL;
}

void pci_enable_device(pci_device_t* dev) {
    if (!dev) {
        return;
    }
    u32 cmd = pci_read_config(dev->bus, dev->device, dev->function, 0x04);
    cmd |= 0x07;
    pci_write_config(dev->bus, dev->device, dev->function, 0x04, cmd);
}

void* pci_map_bar(pci_device_t* dev, u32 bar_index) {
    if (!dev || bar_index >= 6) {
        return NULL;
    }
    u32 bar = dev->bar[bar_index];
    if (bar & 1) {
        return (void*)(uintptr_t)(bar & ~0x3);
    }
    return (void*)(uintptr_t)(bar & ~0xF);
}

void pci_register_driver(u16 vendor_id, u16 device_id, pci_driver_init_t init) {
    pci_device_t* dev = pci_find_device(vendor_id, device_id);
    if (dev && init) {
        init(dev);
    }
}
