#ifndef DRM_H
#define DRM_H

#include "types.h"

/* DRM Device */
typedef struct drm_device drm_device_t;

/* Initialize DRM */
void drm_init(void);

/* Allocate/free DRM device */
drm_device_t* drm_alloc_device(void);
void drm_free_device(drm_device_t* dev);

/* Set display mode */
int drm_set_mode(drm_device_t* dev, u32 width, u32 height, u32 bpp);

/* Get framebuffer */
void* drm_get_framebuffer(drm_device_t* dev);

/* Find device by name */
drm_device_t* drm_find_device(const char* name);

/* Get device count */
u32 drm_get_device_count(void);

#endif /* DRM_H */
