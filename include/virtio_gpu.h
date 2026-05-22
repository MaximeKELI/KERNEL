#ifndef VIRTIO_GPU_H
#define VIRTIO_GPU_H

#include "types.h"

void virtio_gpu_init(void);
bool virtio_gpu_available(void);
void virtio_gpu_flush_rect(u32 x, u32 y, u32 w, u32 h);

#endif /* VIRTIO_GPU_H */
