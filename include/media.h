#ifndef MEDIA_H
#define MEDIA_H

#include "types.h"
#include "codec.h"

/* Initialize media subsystem */
void media_init(void);

/* Detect media format from file extension */
codec_format_t media_detect_format_from_extension(const char* filename);

/* Detect media format from file header (magic bytes) */
codec_format_t media_detect_format_from_header(const void* data, size_t len);

/* Get codec type from format */
codec_type_t media_get_codec_type(codec_format_t format);

/* Get format name as string */
const char* media_format_to_string(codec_format_t format);

#endif /* MEDIA_H */
