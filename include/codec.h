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
    CODEC_FORMAT_OPUS,        /* Modern audio codec */
    CODEC_FORMAT_FLAC,         /* Lossless audio */
    CODEC_FORMAT_PCM,
    CODEC_FORMAT_H264,
    CODEC_FORMAT_H265,
    CODEC_FORMAT_AV1,          /* Modern video codec (AOMedia Video 1) */
    CODEC_FORMAT_VP8,
    CODEC_FORMAT_VP9,
    CODEC_FORMAT_RAW
} codec_format_t;

/* FourCC (Four Character Code) for format identification */
typedef u32 fourcc_t;

/* Common FourCC codes */
#define FOURCC_MP3    0x4D503300  /* "MP3\0" */
#define FOURCC_AAC    0x41414300  /* "AAC\0" */
#define FOURCC_OPUS   0x4F505553  /* "OPUS" */
#define FOURCC_FLAC   0x464C4143  /* "FLAC" */
#define FOURCC_H264   0x48323634  /* "H264" */
#define FOURCC_H265   0x48323635  /* "H265" */
#define FOURCC_AV1    0x41563120  /* "AV1 " */
#define FOURCC_VP8   0x56503820  /* "VP8 " */
#define FOURCC_VP9   0x56503920  /* "VP9 " */
#define FOURCC_MP4   0x4D503420  /* "MP4 " */
#define FOURCC_MKV   0x4D4B5620  /* "MKV " */
#define FOURCC_WEBM  0x5745424D  /* "WEBM" */

/* Container formats */
typedef enum {
    CONTAINER_FORMAT_NONE,
    CONTAINER_FORMAT_MP4,
    CONTAINER_FORMAT_MKV,
    CONTAINER_FORMAT_WEBM,
    CONTAINER_FORMAT_OGG,
    CONTAINER_FORMAT_AVI,
    CONTAINER_FORMAT_RAW
} container_format_t;

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

/* FourCC utilities */
fourcc_t codec_format_to_fourcc(codec_format_t format);
codec_format_t codec_fourcc_to_format(fourcc_t fourcc);
const char* codec_fourcc_to_string(fourcc_t fourcc);

/* Container support */
container_format_t codec_detect_container(const void* data, size_t len);
const char* container_format_to_string(container_format_t format);

#endif /* CODEC_H */
