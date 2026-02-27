#include "video_core.h"
#include "drm.h"
#include "framebuffer.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "validate.h"
#include "vmm.h"

#define MAX_VIDEO_DEVICES 8
#define MAX_VIDEO_MODES 32
#define MAX_VIDEO_BUFFERS 16

/* Video mode */
typedef struct video_mode {
    u32 width;
    u32 height;
    u32 refresh_rate;
    u32 bpp;  /* Bits per pixel */
    bool valid;
} video_mode_t;

/* Video buffer */
typedef struct video_buffer {
    u32 buffer_id;
    void* data;
    u32 width;
    u32 height;
    u32 pitch;
    u32 format;
    u64 size;
    bool active;
    struct video_buffer* next;
} video_buffer_t;

/* Video device */
typedef struct video_device {
    u32 device_id;
    char name[32];
    drm_device_t* drm_dev;
    framebuffer_t* fb;
    video_mode_t modes[MAX_VIDEO_MODES];
    u32 num_modes;
    u32 current_mode;
    video_buffer_t* buffers[MAX_VIDEO_BUFFERS];
    u32 num_buffers;
    u32 active_buffer;
    bool active;
    struct video_device* next;
} video_device_t;

static video_device_t* video_devices = NULL;
static spinlock_t video_global_lock = SPINLOCK_INIT;
static u32 video_buffer_counter = 0;
static u32 video_device_id_counter = 0;  /* Global counter for device IDs */

void video_core_init(void) {
    video_devices = NULL;
    video_buffer_counter = 0;
    printk("[Video Core] Video core subsystem initialized\n");
}

video_device_t* video_device_create(const char* name, drm_device_t* drm_dev) {
    VALIDATE_PTR_RET(name, NULL);
    VALIDATE_PTR_RET(drm_dev, NULL);
    
    video_device_t* dev = (video_device_t*)kzalloc(sizeof(video_device_t));
    if (!dev) {
        return NULL;
    }
    
    /* Generate unique device ID */
    spinlock_lock(&video_global_lock);
    dev->device_id = video_device_id_counter++;
    spinlock_unlock(&video_global_lock);
    
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->name[sizeof(dev->name) - 1] = '\0'; /* Ensure null termination */
    dev->drm_dev = drm_dev;
    dev->num_modes = 0;
    dev->current_mode = 0;
    dev->num_buffers = 0;
    dev->active_buffer = 0;
    dev->active = false;
    
    /* Get framebuffer */
    dev->fb = framebuffer_get();
    if (!dev->fb) {
        /* Initialize default framebuffer */
        framebuffer_init(VESA_MODE_1024x768_8);
        dev->fb = framebuffer_get();
        if (!dev->fb) {
            kfree(dev);
            return NULL;
        }
    }
    
    spinlock_lock(&video_global_lock);
    dev->next = video_devices;
    video_devices = dev;
    spinlock_unlock(&video_global_lock);
    
    DEBUG_INFO("Video device created: %s", name);
    return dev;
}

int video_device_add_mode(video_device_t* dev, u32 width, u32 height, 
                          u32 refresh_rate, u32 bpp) {
    VALIDATE_PTR_RET(dev, -1);
    VALIDATE_RANGE(width, 320, 7680);
    VALIDATE_RANGE(height, 240, 4320);
    VALIDATE_RANGE(refresh_rate, 24, 240);
    VALIDATE_RANGE(bpp, 8, 32);
    
    if (dev->num_modes >= MAX_VIDEO_MODES) {
        return -1;
    }
    
    video_mode_t* mode = &dev->modes[dev->num_modes];
    mode->width = width;
    mode->height = height;
    mode->refresh_rate = refresh_rate;
    mode->bpp = bpp;
    mode->valid = true;
    dev->num_modes++;
    
    DEBUG_INFO("Video mode added: %ux%u@%uHz %ubpp", 
              width, height, refresh_rate, bpp);
    return 0;
}

int video_device_set_mode(video_device_t* dev, u32 mode_index) {
    VALIDATE_PTR_RET(dev, -1);
    
    if (mode_index >= dev->num_modes) {
        return -1;
    }
    
    video_mode_t* mode = &dev->modes[mode_index];
    if (!mode->valid) {
        return -1;
    }
    
    /* Save current mode for rollback */
    u32 old_mode = dev->current_mode;
    framebuffer_t* old_fb = dev->fb;
    
    /* Update framebuffer mode */
    /* Note: framebuffer is global, we just update our reference */
    dev->fb = framebuffer_get();
    if (!dev->fb) {
        return -1;
    }
    
    /* Calculate new framebuffer parameters */
    u32 new_pitch = mode->width * (mode->bpp / 8);
    u64 new_size = (u64)new_pitch * mode->height;
    
    /* Validate new size is reasonable (prevent overflow) */
    if (new_size > (u64)1024 * 1024 * 1024) { /* 1GB max */
        return -1;
    }
    
    /* Update framebuffer dimensions */
    dev->fb->width = mode->width;
    dev->fb->height = mode->height;
    dev->fb->bpp = mode->bpp;
    dev->fb->pitch = new_pitch;
    dev->fb->size = new_size;
    
    /* Only update current_mode if everything succeeded */
    dev->current_mode = mode_index;
    
    DEBUG_INFO("Video mode set: %ux%u@%uHz", 
              mode->width, mode->height, mode->refresh_rate);
    return 0;
}

video_buffer_t* video_buffer_create(video_device_t* dev, u32 width, 
                                    u32 height, u32 format) {
    VALIDATE_PTR_RET(dev, NULL);
    VALIDATE_RANGE(width, 1, 7680);
    VALIDATE_RANGE(height, 1, 4320);
    
    if (dev->num_buffers >= MAX_VIDEO_BUFFERS) {
        return NULL;
    }
    
    video_buffer_t* buf = (video_buffer_t*)kzalloc(sizeof(video_buffer_t));
    if (!buf) {
        return NULL;
    }
    
    buf->buffer_id = video_buffer_counter++;
    buf->width = width;
    buf->height = height;
    buf->format = format;
    buf->pitch = width * 4; /* Assume 32bpp for now */
    buf->size = buf->pitch * height;
    buf->data = vmm_alloc_pages((buf->size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!buf->data) {
        kfree(buf);
        return NULL;
    }
    
    buf->active = false;
    
    dev->buffers[dev->num_buffers++] = buf;
    
    DEBUG_INFO("Video buffer created: id=%u, %ux%u", 
              buf->buffer_id, width, height);
    return buf;
}

void video_buffer_destroy(video_buffer_t* buf) {
    VALIDATE_PTR_VOID(buf);
    
    if (buf->data) {
        size_t pages = (buf->size + PAGE_SIZE - 1) / PAGE_SIZE;
        vmm_free_pages(buf->data, pages);
    }
    
    kfree(buf);
}

int video_buffer_write(video_buffer_t* buf, const void* data, size_t len) {
    VALIDATE_PTR_RET(buf, -1);
    VALIDATE_PTR_RET(data, -1);
    VALIDATE_SIZE(len);
    
    if (len > buf->size) {
        len = buf->size;
    }
    
    memcpy(buf->data, data, len);
    return len;
}

int video_buffer_flip(video_device_t* dev, u32 buffer_id) {
    VALIDATE_PTR_RET(dev, -1);
    
    if (buffer_id >= dev->num_buffers) {
        return -1;
    }
    
    video_buffer_t* buf = dev->buffers[buffer_id];
    if (!buf) {
        return -1;
    }
    
    /* Copy buffer to framebuffer */
    if (dev->fb && buf->data && dev->fb->addr) {
        video_mode_t* mode = &dev->modes[dev->current_mode];
        size_t copy_size = mode->width * mode->height * (mode->bpp / 8);
        if (copy_size > buf->size) {
            copy_size = buf->size;
        }
        if (copy_size > dev->fb->size) {
            copy_size = dev->fb->size;
        }
        memcpy(dev->fb->addr, buf->data, copy_size);
    }
    
    dev->active_buffer = buffer_id;
    
    return 0;
}

int video_device_start(video_device_t* dev) {
    VALIDATE_PTR_RET(dev, -1);
    
    dev->active = true;
    DEBUG_INFO("Video device started: %s", dev->name);
    return 0;
}

int video_device_stop(video_device_t* dev) {
    VALIDATE_PTR_RET(dev, -1);
    
    dev->active = false;
    DEBUG_INFO("Video device stopped: %s", dev->name);
    return 0;
}

video_device_t* video_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&video_global_lock);
    
    video_device_t* dev = video_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&video_global_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&video_global_lock);
    return NULL;
}

u32 video_get_width(video_device_t* dev) {
    VALIDATE_PTR_RET(dev, 0);
    if (dev->current_mode < dev->num_modes) {
        return dev->modes[dev->current_mode].width;
    }
    return 0;
}

u32 video_get_height(video_device_t* dev) {
    VALIDATE_PTR_RET(dev, 0);
    if (dev->current_mode < dev->num_modes) {
        return dev->modes[dev->current_mode].height;
    }
    return 0;
}
