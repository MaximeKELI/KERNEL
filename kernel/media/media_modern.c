#include "media_modern.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "string.h"
#include "validate.h"
#include "spinlock.h"
#include "types.h"

static bool hw_accel_initialized = false;
static spinlock_t hw_accel_lock = SPINLOCK_INIT;

int media_hw_accel_init(void) {
    if (hw_accel_initialized) {
        return 0;
    }
    
    spinlock_init(&hw_accel_lock);
    
    /* TODO: Detect available hardware acceleration */
    /* For now, just mark as initialized */
    hw_accel_initialized = true;
    
    DEBUG_INFO("Hardware acceleration subsystem initialized", 0);
    printk("[Media HW] Hardware acceleration initialized\n");
    
    return 0;
}

int media_hw_accel_get_caps(hw_accel_type_t type, hw_accel_caps_t* caps) {
    VALIDATE_PTR_RET(caps, -1);
    
    if (!hw_accel_initialized) {
        return -1;
    }
    
    /* Initialize caps structure */
    memset(caps, 0, sizeof(hw_accel_caps_t));
    caps->type = type;
    
    /* TODO: Query actual hardware capabilities */
    /* For now, return basic structure */
    switch (type) {
        case HW_ACCEL_VAAPI:
        case HW_ACCEL_NVENC:
        case HW_ACCEL_QUICKSYNC:
            caps->decode_supported = true;
            caps->encode_supported = true;
            caps->supported_formats[0] = CODEC_FORMAT_H264;
            caps->supported_formats[1] = CODEC_FORMAT_H265;
            caps->num_supported_formats = 2;
            caps->max_width = 7680;
            caps->max_height = 4320;
            caps->max_bitrate = 100000000; /* 100 Mbps */
            break;
        default:
            caps->decode_supported = false;
            caps->encode_supported = false;
            break;
    }
    
    return 0;
}

bool media_hw_accel_is_supported(codec_format_t format, hw_accel_type_t type) {
    hw_accel_caps_t caps;
    if (media_hw_accel_get_caps(type, &caps) < 0) {
        return false;
    }
    
    for (u32 i = 0; i < caps.num_supported_formats; i++) {
        if (caps.supported_formats[i] == format) {
            return true;
        }
    }
    
    return false;
}

media_zerocopy_buffer_t* media_zerocopy_buffer_create(size_t size) {
    /* Validate size manually (VALIDATE_SIZE returns -1, not NULL) */
    if (size == 0 || size > (1024 * 1024 * 1024)) { /* Max 1GB */
        return NULL;
    }
    
    media_zerocopy_buffer_t* buf = (media_zerocopy_buffer_t*)kzalloc(sizeof(media_zerocopy_buffer_t));
    if (!buf) {
        return NULL;
    }
    
    buf->size = size;
    buf->dma_fd = -1;
    buf->mapped = false;
    
    /* TODO: Allocate DMA-coherent memory or use special allocator */
    /* For now, use regular allocation */
    buf->virtual_addr = kzalloc(size);
    if (!buf->virtual_addr) {
        kfree(buf);
        return NULL;
    }
    
    /* TODO: Get actual physical address from virtual address */
    /* For now, set to NULL to indicate invalid physical address */
    /* This prevents incorrect use of physical_addr */
    buf->physical_addr = NULL; /* TODO: Implement proper virtual-to-physical mapping */
    
    DEBUG_INFO("Zero-copy buffer created: %zu bytes", size);
    return buf;
}

void media_zerocopy_buffer_destroy(media_zerocopy_buffer_t* buf) {
    VALIDATE_PTR_VOID(buf);
    
    if (buf->virtual_addr) {
        kfree(buf->virtual_addr);
    }
    
    kfree(buf);
}

media_metadata_t* media_metadata_create(void) {
    media_metadata_t* metadata = (media_metadata_t*)kzalloc(sizeof(media_metadata_t));
    if (!metadata) {
        return NULL;
    }
    
    metadata->year = 0;
    metadata->track = 0;
    metadata->duration_ms = 0;
    metadata->bitrate = 0;
    metadata->sample_rate = 0;
    metadata->channels = 0;
    metadata->custom_data = NULL;
    metadata->custom_data_size = 0;
    metadata->next = NULL;
    
    return metadata;
}

void media_metadata_destroy(media_metadata_t* metadata) {
    VALIDATE_PTR_VOID(metadata);
    
    if (metadata->custom_data) {
        kfree(metadata->custom_data);
    }
    
    if (metadata->next) {
        media_metadata_destroy(metadata->next);
    }
    
    kfree(metadata);
}

int media_metadata_set_string(media_metadata_t* metadata, const char* key, const char* value) {
    VALIDATE_PTR_RET(metadata, -1);
    VALIDATE_PTR_RET(key, -1);
    VALIDATE_PTR_RET(value, -1);
    
    if (strcmp(key, "title") == 0) {
        strncpy(metadata->title, value, sizeof(metadata->title) - 1);
        metadata->title[sizeof(metadata->title) - 1] = '\0'; /* Ensure null termination */
        return 0;
    }
    if (strcmp(key, "artist") == 0) {
        strncpy(metadata->artist, value, sizeof(metadata->artist) - 1);
        metadata->artist[sizeof(metadata->artist) - 1] = '\0'; /* Ensure null termination */
        return 0;
    }
    if (strcmp(key, "album") == 0) {
        strncpy(metadata->album, value, sizeof(metadata->album) - 1);
        metadata->album[sizeof(metadata->album) - 1] = '\0'; /* Ensure null termination */
        return 0;
    }
    if (strcmp(key, "genre") == 0) {
        strncpy(metadata->genre, value, sizeof(metadata->genre) - 1);
        metadata->genre[sizeof(metadata->genre) - 1] = '\0'; /* Ensure null termination */
        return 0;
    }
    
    return -1;
}

const char* media_metadata_get_string(media_metadata_t* metadata, const char* key) {
    VALIDATE_PTR_RET(metadata, NULL);
    VALIDATE_PTR_RET(key, NULL);
    
    if (strcmp(key, "title") == 0) {
        return metadata->title;
    }
    if (strcmp(key, "artist") == 0) {
        return metadata->artist;
    }
    if (strcmp(key, "album") == 0) {
        return metadata->album;
    }
    if (strcmp(key, "genre") == 0) {
        return metadata->genre;
    }
    
    return NULL;
}

int media_metadata_set_int(media_metadata_t* metadata, const char* key, u32 value) {
    VALIDATE_PTR_RET(metadata, -1);
    VALIDATE_PTR_RET(key, -1);
    
    if (strcmp(key, "year") == 0) {
        metadata->year = value;
        return 0;
    }
    if (strcmp(key, "track") == 0) {
        metadata->track = value;
        return 0;
    }
    if (strcmp(key, "duration_ms") == 0) {
        metadata->duration_ms = value;
        return 0;
    }
    if (strcmp(key, "bitrate") == 0) {
        metadata->bitrate = value;
        return 0;
    }
    if (strcmp(key, "sample_rate") == 0) {
        metadata->sample_rate = value;
        return 0;
    }
    if (strcmp(key, "channels") == 0) {
        metadata->channels = value;
        return 0;
    }
    
    return -1;
}

u32 media_metadata_get_int(media_metadata_t* metadata, const char* key) {
    VALIDATE_PTR_RET(metadata, 0);
    VALIDATE_PTR_RET(key, 0);
    
    if (strcmp(key, "year") == 0) {
        return metadata->year;
    }
    if (strcmp(key, "track") == 0) {
        return metadata->track;
    }
    if (strcmp(key, "duration_ms") == 0) {
        return metadata->duration_ms;
    }
    if (strcmp(key, "bitrate") == 0) {
        return metadata->bitrate;
    }
    if (strcmp(key, "sample_rate") == 0) {
        return metadata->sample_rate;
    }
    if (strcmp(key, "channels") == 0) {
        return metadata->channels;
    }
    
    return 0;
}

int media_async_submit(media_async_io_t* async_io) {
    VALIDATE_PTR_RET(async_io, -1);
    VALIDATE_PTR_RET(async_io->buffer, -1);
    
    if (async_io->size == 0) {
        return -1;
    }
    
    /* Check if already submitted */
    if (async_io->completed == false) {
        return -1; /* Already in progress */
    }
    
    async_io->completed = false;
    
    /* TODO: Submit to async I/O subsystem */
    /* For now, mark as completed immediately (simulate success) */
    int result = 0; /* 0 = success, -1 = error */
    size_t bytes_processed = async_io->size;
    
    /* Simulate async completion */
    async_io->completed = true;
    
    if (async_io->callback) {
        async_io->callback(async_io->user_data, result, bytes_processed);
    }
    
    return result;
}

int media_async_wait(media_async_io_t* async_io, u32 timeout_ms) {
    VALIDATE_PTR_RET(async_io, -1);
    
    (void)timeout_ms; /* TODO: Use timeout in proper implementation */
    
    /* TODO: Implement proper waiting mechanism */
    /* For now, just check if completed */
    if (async_io->completed) {
        return 0;
    }
    
    return -1;
}

int media_async_cancel(media_async_io_t* async_io) {
    VALIDATE_PTR_RET(async_io, -1);
    
    /* TODO: Implement cancellation */
    async_io->completed = true;
    
    return 0;
}
