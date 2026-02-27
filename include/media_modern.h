#ifndef MEDIA_MODERN_H
#define MEDIA_MODERN_H

#include "types.h"
#include "codec.h"

/* Hardware acceleration types */
typedef enum {
    HW_ACCEL_NONE,
    HW_ACCEL_VAAPI,      /* Video Acceleration API (Linux) */
    HW_ACCEL_VDPAU,      /* Video Decode and Presentation API */
    HW_ACCEL_NVENC,      /* NVIDIA Encoder */
    HW_ACCEL_QUICKSYNC,  /* Intel Quick Sync Video */
    HW_ACCEL_VIDEOTOOLBOX /* macOS VideoToolbox */
} hw_accel_type_t;

/* Hardware acceleration capabilities */
typedef struct {
    hw_accel_type_t type;
    bool decode_supported;
    bool encode_supported;
    codec_format_t supported_formats[16];
    u32 num_supported_formats;
    u32 max_width;
    u32 max_height;
    u32 max_bitrate;
} hw_accel_caps_t;

/* Metadata structure (modern tags) */
typedef struct media_metadata {
    char title[256];
    char artist[256];
    char album[256];
    char genre[64];
    u32 year;
    u32 track;
    u32 duration_ms;
    u32 bitrate;
    u32 sample_rate;
    u32 channels;
    void* custom_data;
    size_t custom_data_size;
    struct media_metadata* next;
} media_metadata_t;

/* Zero-copy buffer for media operations */
typedef struct media_zerocopy_buffer {
    void* physical_addr;
    void* virtual_addr;
    size_t size;
    u32 dma_fd;  /* DMA buffer file descriptor (if applicable) */
    bool mapped;
} media_zerocopy_buffer_t;

/* Initialize hardware acceleration */
int media_hw_accel_init(void);

/* Get hardware acceleration capabilities */
int media_hw_accel_get_caps(hw_accel_type_t type, hw_accel_caps_t* caps);

/* Check if format is supported by hardware */
bool media_hw_accel_is_supported(codec_format_t format, hw_accel_type_t type);

/* Create zero-copy buffer */
media_zerocopy_buffer_t* media_zerocopy_buffer_create(size_t size);

/* Destroy zero-copy buffer */
void media_zerocopy_buffer_destroy(media_zerocopy_buffer_t* buf);

/* Metadata functions */
media_metadata_t* media_metadata_create(void);
void media_metadata_destroy(media_metadata_t* metadata);
int media_metadata_set_string(media_metadata_t* metadata, const char* key, const char* value);
const char* media_metadata_get_string(media_metadata_t* metadata, const char* key);
int media_metadata_set_int(media_metadata_t* metadata, const char* key, u32 value);
u32 media_metadata_get_int(media_metadata_t* metadata, const char* key);

/* Async I/O for media operations */
typedef void (*media_async_callback_t)(void* user_data, int result, size_t bytes_processed);

typedef struct media_async_io {
    void* buffer;
    size_t size;
    media_async_callback_t callback;
    void* user_data;
    bool completed;
} media_async_io_t;

/* Submit async I/O operation */
int media_async_submit(media_async_io_t* async_io);

/* Wait for async I/O completion */
int media_async_wait(media_async_io_t* async_io, u32 timeout_ms);

/* Cancel async I/O operation */
int media_async_cancel(media_async_io_t* async_io);

#endif /* MEDIA_MODERN_H */
