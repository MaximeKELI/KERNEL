#include "codec.h"
#include "audio_core.h"
#include "video_core.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "validate.h"

#define MAX_CODECS 32
#define CODEC_BUFFER_SIZE (256 * 1024)  /* 256KB */

/* Codec */
typedef struct codec {
    u32 codec_id;
    char name[32];
    codec_type_t type;
    codec_format_t format;
    void* decoder_state;
    void* encoder_state;
    void* buffer;
    size_t buffer_size;
    bool active;
    struct codec* next;
} codec_t;

/* Codec operations */
typedef struct codec_ops {
    int (*decode)(codec_t* codec, const void* input, size_t input_len,
                  void* output, size_t* output_len);
    int (*encode)(codec_t* codec, const void* input, size_t input_len,
                  void* output, size_t* output_len);
    int (*init)(codec_t* codec);
    void (*cleanup)(codec_t* codec);
} codec_ops_t;

static codec_t* codecs = NULL;
static spinlock_t codec_lock = SPINLOCK_INIT;
static u32 codec_counter = 0;

void codec_init(void) {
    codecs = NULL;
    codec_counter = 0;
    printk("[Codec] Codec subsystem initialized\n");
}

/* PCM codec (passthrough) */
static int pcm_decode(codec_t* codec, const void* input, size_t input_len,
                      void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len) {
        return -1;
    }
    
    if (input_len > *output_len) {
        input_len = *output_len;
    }
    
    memcpy(output, input, input_len);
    *output_len = input_len;
    
    return 0;
}

static int pcm_encode(codec_t* codec, const void* input, size_t input_len,
                      void* output, size_t* output_len) {
    return pcm_decode(codec, input, input_len, output, output_len);
}

static int pcm_init(codec_t* codec) {
    (void)codec;
    return 0;
}

static void pcm_cleanup(codec_t* codec) {
    (void)codec;
}

static codec_ops_t pcm_ops = {
    .decode = pcm_decode,
    .encode = pcm_encode,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* MP3 decoder stub (simplified - validates format, returns PCM passthrough for now) */
static int mp3_decode(codec_t* codec, const void* input, size_t input_len,
                      void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic MP3 header validation (ID3v2 or frame sync) */
    const u8* in = (const u8*)input;
    bool valid_mp3 = false;
    
    if (input_len >= 3) {
        /* Check for ID3v2 tag */
        if (in[0] == 'I' && in[1] == 'D' && in[2] == '3') {
            valid_mp3 = true;
        }
        /* Check for MP3 frame sync (11 bits set: 0xFFE) */
        else if (input_len >= 2 && (in[0] == 0xFF && (in[1] & 0xE0) == 0xE0)) {
            valid_mp3 = true;
        }
    }
    
    if (!valid_mp3 && input_len > 0) {
        /* For now, pass through as PCM if format not clearly MP3 */
        DEBUG_INFO("MP3: Format validation unclear, using passthrough", 0);
    }
    
    /* Simplified: passthrough for now (full MP3 decode would require libmp3lame or similar) */
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("MP3 decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t mp3_ops = {
    .decode = mp3_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* AAC decoder stub */
static int aac_decode(codec_t* codec, const void* input, size_t input_len,
                      void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic AAC ADTS header validation */
    const u8* in = (const u8*)input;
    bool valid_aac = false;
    
    if (input_len >= 7) {
        /* ADTS header: syncword (12 bits: 0xFFF) */
        if (in[0] == 0xFF && (in[1] & 0xF0) == 0xF0) {
            valid_aac = true;
        }
    }
    
    if (!valid_aac && input_len > 0) {
        DEBUG_INFO("AAC: Format validation unclear, using passthrough", 0);
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("AAC decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t aac_ops = {
    .decode = aac_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* OGG Vorbis decoder stub */
static int ogg_vorbis_decode(codec_t* codec, const void* input, size_t input_len,
                             void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic OGG page validation */
    const u8* in = (const u8*)input;
    bool valid_ogg = false;
    
    if (input_len >= 4) {
        /* OGG page starts with "OggS" */
        if (in[0] == 'O' && in[1] == 'g' && in[2] == 'g' && in[3] == 'S') {
            valid_ogg = true;
        }
    }
    
    if (!valid_ogg && input_len > 0) {
        DEBUG_INFO("OGG Vorbis: Format validation unclear, using passthrough", 0);
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("OGG Vorbis decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t ogg_vorbis_ops = {
    .decode = ogg_vorbis_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* H.264 decoder stub (validates NAL units) */
static int h264_decode(codec_t* codec, const void* input, size_t input_len,
                       void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic H.264 NAL unit validation */
    const u8* in = (const u8*)input;
    bool valid_h264 = false;
    
    if (input_len >= 4) {
        /* Check for start code: 0x00 0x00 0x00 0x01 or 0x00 0x00 0x01 */
        if ((in[0] == 0x00 && in[1] == 0x00 && in[2] == 0x00 && in[3] == 0x01) ||
            (in[0] == 0x00 && in[1] == 0x00 && in[2] == 0x01)) {
            valid_h264 = true;
        }
        /* Check for Annex B format or AVCC format */
        else if (input_len >= 1 && (in[0] & 0x1F) <= 23) {
            /* Valid NAL unit type */
            valid_h264 = true;
        }
    }
    
    if (!valid_h264 && input_len > 0) {
        DEBUG_INFO("H.264: Format validation unclear, using passthrough", 0);
    }
    
    /* Simplified: passthrough for now (full H.264 decode would require libx264 or similar) */
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("H.264 decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t h264_ops = {
    .decode = h264_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* H.265/HEVC decoder stub */
static int h265_decode(codec_t* codec, const void* input, size_t input_len,
                       void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic H.265 NAL unit validation */
    const u8* in = (const u8*)input;
    bool valid_h265 = false;
    
    if (input_len >= 4) {
        /* Check for start code */
        if ((in[0] == 0x00 && in[1] == 0x00 && in[2] == 0x00 && in[3] == 0x01) ||
            (in[0] == 0x00 && in[1] == 0x00 && in[2] == 0x01)) {
            valid_h265 = true;
        }
        /* Check for valid NAL unit type (H.265 uses 6 bits) */
        else if (input_len >= 2 && ((in[0] >> 1) & 0x3F) <= 63) {
            valid_h265 = true;
        }
    }
    
    if (!valid_h265 && input_len > 0) {
        DEBUG_INFO("H.265: Format validation unclear, using passthrough", 0);
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("H.265 decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t h265_ops = {
    .decode = h265_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* VP8 decoder stub */
static int vp8_decode(codec_t* codec, const void* input, size_t input_len,
                       void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic VP8 frame validation */
    if (input_len >= 10) {
        /* VP8 keyframe starts with specific pattern */
        /* Simplified check - accept for now */
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("VP8 decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t vp8_ops = {
    .decode = vp8_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* VP9 decoder stub */
static int vp9_decode(codec_t* codec, const void* input, size_t input_len,
                      void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic VP9 frame validation */
    if (input_len >= 1) {
        /* VP9 frame header validation */
        /* Simplified check - accept for now */
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("VP9 decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t vp9_ops = {
    .decode = vp9_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* RAW format passthrough */
static codec_ops_t raw_ops = {
    .decode = pcm_decode,
    .encode = pcm_encode,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

codec_t* codec_create(const char* name, codec_type_t type, codec_format_t format) {
    VALIDATE_PTR_RET(name, NULL);
    
    codec_t* codec = (codec_t*)kzalloc(sizeof(codec_t));
    if (!codec) {
        return NULL;
    }
    
    codec->codec_id = codec_counter++;
    strncpy(codec->name, name, sizeof(codec->name) - 1);
    codec->name[sizeof(codec->name) - 1] = '\0'; /* Ensure null termination */
    codec->type = type;
    codec->format = format;
    codec->buffer_size = CODEC_BUFFER_SIZE;
    codec->buffer = kzalloc(codec->buffer_size);
    if (!codec->buffer) {
        kfree(codec);
        return NULL;
    }
    
    /* Select codec operations */
    codec_ops_t* ops = NULL;
    switch (format) {
        case CODEC_FORMAT_PCM:
            ops = &pcm_ops;
            break;
        case CODEC_FORMAT_MP3:
            ops = &mp3_ops;
            break;
        case CODEC_FORMAT_AAC:
            ops = &aac_ops;
            break;
        case CODEC_FORMAT_OGG_VORBIS:
            ops = &ogg_vorbis_ops;
            break;
        case CODEC_FORMAT_OPUS:
            ops = &opus_ops;
            break;
        case CODEC_FORMAT_FLAC:
            ops = &flac_ops;
            break;
        case CODEC_FORMAT_H264:
            ops = &h264_ops;
            break;
        case CODEC_FORMAT_H265:
            ops = &h265_ops;
            break;
        case CODEC_FORMAT_AV1:
            ops = &av1_ops;
            break;
        case CODEC_FORMAT_VP8:
            ops = &vp8_ops;
            break;
        case CODEC_FORMAT_VP9:
            ops = &vp9_ops;
            break;
        case CODEC_FORMAT_RAW:
            ops = &raw_ops;
            break;
        default:
            DEBUG_ERROR("Unsupported codec format: %u", format);
            kfree(codec->buffer);
            kfree(codec);
            return NULL;
    }
    
    /* Initialize codec */
    if (ops->init) {
        if (ops->init(codec) < 0) {
            kfree(codec->buffer);
            kfree(codec);
            return NULL;
        }
    }
    
    codec->active = true;
    
    spinlock_lock(&codec_lock);
    codec->next = codecs;
    codecs = codec;
    spinlock_unlock(&codec_lock);
    
    DEBUG_INFO("Codec created: %s, type=%u, format=%u", name, type, format);
    return codec;
}

void codec_destroy(codec_t* codec) {
    VALIDATE_PTR_VOID(codec);
    
    spinlock_lock(&codec_lock);
    
    /* Remove from list */
    if (codecs == codec) {
        codecs = codec->next;
    } else {
        codec_t* current = codecs;
        while (current && current->next != codec) {
            current = current->next;
        }
        if (current) {
            current->next = codec->next;
        }
    }
    
    spinlock_unlock(&codec_lock);
    
    /* Cleanup */
    codec_ops_t* ops = NULL;
    switch (codec->format) {
        case CODEC_FORMAT_PCM:
            ops = &pcm_ops;
            break;
        case CODEC_FORMAT_MP3:
            ops = &mp3_ops;
            break;
        case CODEC_FORMAT_AAC:
            ops = &aac_ops;
            break;
        case CODEC_FORMAT_OGG_VORBIS:
            ops = &ogg_vorbis_ops;
            break;
        case CODEC_FORMAT_OPUS:
            ops = &opus_ops;
            break;
        case CODEC_FORMAT_FLAC:
            ops = &flac_ops;
            break;
        case CODEC_FORMAT_H264:
            ops = &h264_ops;
            break;
        case CODEC_FORMAT_H265:
            ops = &h265_ops;
            break;
        case CODEC_FORMAT_AV1:
            ops = &av1_ops;
            break;
        case CODEC_FORMAT_VP8:
            ops = &vp8_ops;
            break;
        case CODEC_FORMAT_VP9:
            ops = &vp9_ops;
            break;
        case CODEC_FORMAT_RAW:
            ops = &raw_ops;
            break;
        default:
            break;
    }
    
    if (ops && ops->cleanup) {
        ops->cleanup(codec);
    }
    
    if (codec->buffer) {
        kfree(codec->buffer);
    }
    
    kfree(codec);
}

int codec_decode(codec_t* codec, const void* input, size_t input_len,
                 void* output, size_t* output_len) {
    VALIDATE_PTR_RET(codec, -1);
    VALIDATE_PTR_RET(input, -1);
    VALIDATE_PTR_RET(output, -1);
    VALIDATE_PTR_RET(output_len, -1);
    
    if (!codec->active) {
        return -1;
    }
    
    codec_ops_t* ops = NULL;
    switch (codec->format) {
        case CODEC_FORMAT_PCM:
            ops = &pcm_ops;
            break;
        case CODEC_FORMAT_MP3:
            ops = &mp3_ops;
            break;
        case CODEC_FORMAT_AAC:
            ops = &aac_ops;
            break;
        case CODEC_FORMAT_OGG_VORBIS:
            ops = &ogg_vorbis_ops;
            break;
        case CODEC_FORMAT_H264:
            ops = &h264_ops;
            break;
        case CODEC_FORMAT_H265:
            ops = &h265_ops;
            break;
        case CODEC_FORMAT_VP8:
            ops = &vp8_ops;
            break;
        case CODEC_FORMAT_VP9:
            ops = &vp9_ops;
            break;
        case CODEC_FORMAT_RAW:
            ops = &raw_ops;
            break;
        default:
            return -1;
    }
    
    if (!ops || !ops->decode) {
        return -1;
    }
    
    return ops->decode(codec, input, input_len, output, output_len);
}

int codec_encode(codec_t* codec, const void* input, size_t input_len,
                 void* output, size_t* output_len) {
    VALIDATE_PTR_RET(codec, -1);
    VALIDATE_PTR_RET(input, -1);
    VALIDATE_PTR_RET(output, -1);
    VALIDATE_PTR_RET(output_len, -1);
    
    if (!codec->active) {
        return -1;
    }
    
    codec_ops_t* ops = NULL;
    switch (codec->format) {
        case CODEC_FORMAT_PCM:
            ops = &pcm_ops;
            break;
        default:
            return -1; /* Encoding not supported for other formats yet */
    }
    
    if (!ops || !ops->encode) {
        return -1;
    }
    
    return ops->encode(codec, input, input_len, output, output_len);
}

codec_t* codec_find(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&codec_lock);
    
    codec_t* codec = codecs;
    while (codec) {
        if (strcmp(codec->name, name) == 0) {
            spinlock_unlock(&codec_lock);
            return codec;
        }
        codec = codec->next;
    }
    
    spinlock_unlock(&codec_lock);
    return NULL;
}

/* Opus decoder stub (modern audio codec) */
static int opus_decode(codec_t* codec, const void* input, size_t input_len,
                        void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic Opus packet validation */
    const u8* in = (const u8*)input;
    bool valid_opus = false;
    
    if (input_len >= 1) {
        /* Opus packet starts with TOC (Table of Contents) byte */
        u8 toc = in[0];
        u8 config = (toc >> 3) & 0x1F;
        if (config <= 18) { /* Valid Opus configuration */
            valid_opus = true;
        }
    }
    
    if (!valid_opus && input_len > 0) {
        DEBUG_INFO("Opus: Format validation unclear, using passthrough", 0);
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("Opus decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t opus_ops = {
    .decode = opus_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* FLAC decoder stub (lossless audio) */
static int flac_decode(codec_t* codec, const void* input, size_t input_len,
                        void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic FLAC header validation */
    const u8* in = (const u8*)input;
    bool valid_flac = false;
    
    if (input_len >= 4) {
        /* FLAC signature: "fLaC" */
        if (in[0] == 'f' && in[1] == 'L' && in[2] == 'a' && in[3] == 'C') {
            valid_flac = true;
        }
    }
    
    if (!valid_flac && input_len > 0) {
        DEBUG_INFO("FLAC: Format validation unclear, using passthrough", 0);
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("FLAC decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t flac_ops = {
    .decode = flac_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* AV1 decoder stub (modern video codec - AOMedia Video 1) */
static int av1_decode(codec_t* codec, const void* input, size_t input_len,
                       void* output, size_t* output_len) {
    (void)codec;
    
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }
    
    /* Basic AV1 OBU (Open Bitstream Unit) validation */
    const u8* in = (const u8*)input;
    bool valid_av1 = false;
    
    if (input_len >= 1) {
        /* AV1 OBU header */
        u8 obu_header = in[0];
        u8 obu_type = (obu_header >> 3) & 0x1F;
        if (obu_type <= 8) { /* Valid OBU types */
            valid_av1 = true;
        }
    }
    
    if (!valid_av1 && input_len > 0) {
        DEBUG_INFO("AV1: Format validation unclear, using passthrough", 0);
    }
    
    size_t copy_len = (input_len < *output_len) ? input_len : *output_len;
    memcpy(output, input, copy_len);
    *output_len = copy_len;
    
    DEBUG_INFO("AV1 decode: %zu bytes -> %zu bytes (passthrough mode)", input_len, copy_len);
    return 0;
}

static codec_ops_t av1_ops = {
    .decode = av1_decode,
    .encode = NULL,
    .init = pcm_init,
    .cleanup = pcm_cleanup
};

/* FourCC utilities */
fourcc_t codec_format_to_fourcc(codec_format_t format) {
    switch (format) {
        case CODEC_FORMAT_MP3:
            return FOURCC_MP3;
        case CODEC_FORMAT_AAC:
            return FOURCC_AAC;
        case CODEC_FORMAT_OPUS:
            return FOURCC_OPUS;
        case CODEC_FORMAT_FLAC:
            return FOURCC_FLAC;
        case CODEC_FORMAT_H264:
            return FOURCC_H264;
        case CODEC_FORMAT_H265:
            return FOURCC_H265;
        case CODEC_FORMAT_AV1:
            return FOURCC_AV1;
        case CODEC_FORMAT_VP8:
            return FOURCC_VP8;
        case CODEC_FORMAT_VP9:
            return FOURCC_VP9;
        default:
            return 0;
    }
}

codec_format_t codec_fourcc_to_format(fourcc_t fourcc) {
    switch (fourcc) {
        case FOURCC_MP3:
            return CODEC_FORMAT_MP3;
        case FOURCC_AAC:
            return CODEC_FORMAT_AAC;
        case FOURCC_OPUS:
            return CODEC_FORMAT_OPUS;
        case FOURCC_FLAC:
            return CODEC_FORMAT_FLAC;
        case FOURCC_H264:
            return CODEC_FORMAT_H264;
        case FOURCC_H265:
            return CODEC_FORMAT_H265;
        case FOURCC_AV1:
            return CODEC_FORMAT_AV1;
        case FOURCC_VP8:
            return CODEC_FORMAT_VP8;
        case FOURCC_VP9:
            return CODEC_FORMAT_VP9;
        default:
            return CODEC_FORMAT_RAW;
    }
}

const char* codec_fourcc_to_string(fourcc_t fourcc) {
    static char fourcc_str[5];
    fourcc_str[0] = (fourcc >> 24) & 0xFF;
    fourcc_str[1] = (fourcc >> 16) & 0xFF;
    fourcc_str[2] = (fourcc >> 8) & 0xFF;
    fourcc_str[3] = fourcc & 0xFF;
    fourcc_str[4] = '\0';
    return fourcc_str;
}

/* Container format detection */
container_format_t codec_detect_container(const void* data, size_t len) {
    if (!data || len < 12) {
        return CONTAINER_FORMAT_RAW;
    }
    
    const u8* bytes = (const u8*)data;
    
    /* MP4: ftyp box at offset 4 */
    if (len >= 12) {
        if (bytes[4] == 'f' && bytes[5] == 't' && bytes[6] == 'y' && bytes[7] == 'p') {
            return CONTAINER_FORMAT_MP4;
        }
    }
    
    /* MKV/WebM: EBML header */
    if (len >= 4) {
        if (bytes[0] == 0x1A && bytes[1] == 0x45 && bytes[2] == 0xDF && bytes[3] == 0xA3) {
            /* Check for WebM specific signature */
            if (len >= 12 && bytes[8] == 0x42 && bytes[9] == 0x82 &&
                bytes[10] == 0x77 && bytes[11] == 0x65) {
                return CONTAINER_FORMAT_WEBM;
            }
            return CONTAINER_FORMAT_MKV;
        }
    }
    
    /* OGG: OggS header */
    if (len >= 4 && bytes[0] == 'O' && bytes[1] == 'g' && 
        bytes[2] == 'g' && bytes[3] == 'S') {
        return CONTAINER_FORMAT_OGG;
    }
    
    /* AVI: RIFF...AVI */
    if (len >= 12 && bytes[0] == 'R' && bytes[1] == 'I' && 
        bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'A' && bytes[9] == 'V' && 
        bytes[10] == 'I' && bytes[11] == ' ') {
        return CONTAINER_FORMAT_AVI;
    }
    
    return CONTAINER_FORMAT_RAW;
}

const char* container_format_to_string(container_format_t format) {
    switch (format) {
        case CONTAINER_FORMAT_MP4:
            return "MP4";
        case CONTAINER_FORMAT_MKV:
            return "MKV";
        case CONTAINER_FORMAT_WEBM:
            return "WebM";
        case CONTAINER_FORMAT_OGG:
            return "OGG";
        case CONTAINER_FORMAT_AVI:
            return "AVI";
        case CONTAINER_FORMAT_RAW:
            return "RAW";
        default:
            return "Unknown";
    }
}
