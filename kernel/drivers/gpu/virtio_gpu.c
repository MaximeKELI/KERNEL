#include "stdio.h"
#include "pci.h"
#include "framebuffer.h"
#include "types.h"

#define VIRTIO_GPU_VENDOR 0x1AF4
#define VIRTIO_GPU_DEVICE 0x1050

static bool virtio_gpu_present = false;

void virtio_gpu_init(void) {
    pci_device_t* dev = pci_find_device(VIRTIO_GPU_VENDOR, VIRTIO_GPU_DEVICE);
    if (!dev) {
        dev = pci_find_class(0x03, 0x00);
    }
    virtio_gpu_present = (dev != NULL);
    printk("[virtio-gpu] %s\n", virtio_gpu_present ? "detected (stub scanout)" : "not present");
}

bool virtio_gpu_available(void) {
    return virtio_gpu_present;
}

void virtio_gpu_flush_rect(u32 x, u32 y, u32 w, u32 h) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    /* Future: virtio GPU resource flush to display */
}
