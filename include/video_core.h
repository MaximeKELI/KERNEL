#ifndef VIDEO_CORE_H
#define VIDEO_CORE_H

#include "types.h"
#include "drm.h"
#include "framebuffer.h"

/* Video formats */
#define VIDEO_FORMAT_RGB888  0x01
#define VIDEO_FORMAT_RGBA8888 0x02
#define VIDEO_FORMAT_YUV420  0x03
#define VIDEO_FORMAT_YUV422  0x04
#define VIDEO_FORMAT_YUV444  0x05

/* Video device */
typedef struct video_device video_device_t;

/* Video buffer */
typedef struct video_buffer video_buffer_t;

/* Initialize video core */
void video_core_init(void);
void video_core_setup_default(void);

/* Create/destroy video device */
video_device_t* video_device_create(const char* name, drm_device_t* drm_dev);
void video_device_destroy(video_device_t* dev);

/* Video modes */
int video_device_add_mode(video_device_t* dev, u32 width, u32 height,
                          u32 refresh_rate, u32 bpp);
int video_device_set_mode(video_device_t* dev, u32 mode_index);

/* Video buffers */
video_buffer_t* video_buffer_create(video_device_t* dev, u32 width,
                                    u32 height, u32 format);
void video_buffer_destroy(video_buffer_t* buf);
int video_buffer_write(video_buffer_t* buf, const void* data, size_t len);
int video_buffer_flip(video_device_t* dev, u32 buffer_id);

/* Control video device */
int video_device_start(video_device_t* dev);
int video_device_stop(video_device_t* dev);

/* Get video device properties */
u32 video_get_width(video_device_t* dev);
u32 video_get_height(video_device_t* dev);
u32 video_get_current_mode(video_device_t* dev);

/* Find video device */
video_device_t* video_find_device(const char* name);

#endif /* VIDEO_CORE_H */
