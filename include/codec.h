#ifndef CODEC_H
#define CODEC_H

#include "types.h"

/* Codec type */
typedef enum {
    CODEC_TYPE_AUDIO,
    CODEC_TYPE_VIDEO
} codec_type_t;

/* Codec format */
typedef enum {
    CODEC_FORMAT_MP3,
    CODEC_FORMAT_AAC,
    CODEC_FORMAT_OGG_VORBIS,
    CODEC_FORMAT_PCM,
    CODEC_FORMAT_H264,
    CODEC_FORMAT_H265,
    CODEC_FORMAT_VP8,
    CODEC_FORMAT_VP9,
    CODEC_FORMAT_RAW
} codec_format_t;

/* Codec */
typedef struct codec codec_t;

/* Initialize codec subsystem */
void codec_init(void);

/* Create/destroy codec */
codec_t* codec_create(const char* name, codec_type_t type, codec_format_t format);
void codec_destroy(codec_t* codec);

/* Decode/encode */
int codec_decode(codec_t* codec, const void* input, size_t input_len,
                 void* output, size_t* output_len);
int codec_encode(codec_t* codec, const void* input, size_t input_len,
                 void* output, size_t* output_len);

/* Find codec */
codec_t* codec_find(const char* name);

#endif /* CODEC_H */
