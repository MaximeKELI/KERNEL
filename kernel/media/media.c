#include "media.h"
#include "audio_core.h"
#include "video_core.h"
#include "codec.h"
#include "av_sync.h"
#include "streaming.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "string.h"
#include "types.h"
#include "lib/string.h"

void media_init(void) {
    /* Initialize all media subsystems */
    audio_core_init();
    video_core_init();
    codec_init();
    av_sync_init();
    streaming_init();
    
    DEBUG_INFO("Media subsystem initialized", 0);
    printk("[Media] Complete multimedia subsystem initialized\n");
}

/* Detect media format from file extension */
codec_format_t media_detect_format_from_extension(const char* filename) {
    if (!filename) {
        return CODEC_FORMAT_RAW;
    }
    
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        return CODEC_FORMAT_RAW;
    }
    ext++; /* Skip the dot */
    
    /* Audio formats */
    if (strcmp(ext, "mp3") == 0 || strcmp(ext, "MP3") == 0) {
        return CODEC_FORMAT_MP3;
    }
    if (strcmp(ext, "aac") == 0 || strcmp(ext, "AAC") == 0 || 
        strcmp(ext, "m4a") == 0 || strcmp(ext, "M4A") == 0) {
        return CODEC_FORMAT_AAC;
    }
    if (strcmp(ext, "ogg") == 0 || strcmp(ext, "OGG") == 0 ||
        strcmp(ext, "oga") == 0 || strcmp(ext, "OGA") == 0) {
        return CODEC_FORMAT_OGG_VORBIS;
    }
    if (strcmp(ext, "wav") == 0 || strcmp(ext, "WAV") == 0 ||
        strcmp(ext, "pcm") == 0 || strcmp(ext, "PCM") == 0) {
        return CODEC_FORMAT_PCM;
    }
    
    /* Video formats */
    if (strcmp(ext, "h264") == 0 || strcmp(ext, "H264") == 0 ||
        strcmp(ext, "264") == 0) {
        return CODEC_FORMAT_H264;
    }
    if (strcmp(ext, "h265") == 0 || strcmp(ext, "H265") == 0 ||
        strcmp(ext, "hevc") == 0 || strcmp(ext, "HEVC") == 0 ||
        strcmp(ext, "265") == 0) {
        return CODEC_FORMAT_H265;
    }
    if (strcmp(ext, "vp8") == 0 || strcmp(ext, "VP8") == 0) {
        return CODEC_FORMAT_VP8;
    }
    if (strcmp(ext, "vp9") == 0 || strcmp(ext, "VP9") == 0) {
        return CODEC_FORMAT_VP9;
    }
    
    return CODEC_FORMAT_RAW;
}

/* Detect media format from file header (magic bytes) */
codec_format_t media_detect_format_from_header(const void* data, size_t len) {
    if (!data || len < 4) {
        return CODEC_FORMAT_RAW;
    }
    
    const u8* bytes = (const u8*)data;
    
    /* MP3: ID3v2 tag or frame sync */
    if (len >= 3 && bytes[0] == 'I' && bytes[1] == 'D' && bytes[2] == '3') {
        return CODEC_FORMAT_MP3;
    }
    if (len >= 2 && bytes[0] == 0xFF && (bytes[1] & 0xE0) == 0xE0) {
        return CODEC_FORMAT_MP3;
    }
    
    /* AAC: ADTS header */
    if (len >= 2 && bytes[0] == 0xFF && (bytes[1] & 0xF0) == 0xF0) {
        /* Check if it's AAC (not MP3) */
        if ((bytes[1] & 0x06) != 0x00) {
            return CODEC_FORMAT_AAC;
        }
    }
    
    /* OGG Vorbis: OggS header */
    if (len >= 4 && bytes[0] == 'O' && bytes[1] == 'g' && 
        bytes[2] == 'g' && bytes[3] == 'S') {
        return CODEC_FORMAT_OGG_VORBIS;
    }
    
    /* H.264: NAL unit start code */
    if (len >= 4) {
        if ((bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x01) ||
            (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01)) {
            /* Check NAL unit type */
            u8 nal_type = (len >= 5) ? (bytes[4] & 0x1F) : 0;
            if (nal_type <= 23) {
                return CODEC_FORMAT_H264;
            }
        }
    }
    
    /* H.265: Similar to H.264 but different NAL unit type */
    if (len >= 4) {
        if ((bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x01) ||
            (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01)) {
            if (len >= 2) {
                u8 nal_type = (bytes[4] >> 1) & 0x3F;
                if (nal_type <= 63) {
                    return CODEC_FORMAT_H265;
                }
            }
        }
    }
    
    /* WAV: RIFF header */
    if (len >= 4 && bytes[0] == 'R' && bytes[1] == 'I' && 
        bytes[2] == 'F' && bytes[3] == 'F') {
        return CODEC_FORMAT_PCM;
    }
    
    return CODEC_FORMAT_RAW;
}

/* Get codec type from format */
codec_type_t media_get_codec_type(codec_format_t format) {
    switch (format) {
        case CODEC_FORMAT_MP3:
        case CODEC_FORMAT_AAC:
        case CODEC_FORMAT_OGG_VORBIS:
        case CODEC_FORMAT_PCM:
            return CODEC_TYPE_AUDIO;
        case CODEC_FORMAT_H264:
        case CODEC_FORMAT_H265:
        case CODEC_FORMAT_VP8:
        case CODEC_FORMAT_VP9:
            return CODEC_TYPE_VIDEO;
        default:
            return CODEC_TYPE_AUDIO; /* Default to audio */
    }
}

/* Get format name as string */
const char* media_format_to_string(codec_format_t format) {
    switch (format) {
        case CODEC_FORMAT_MP3:
            return "MP3";
        case CODEC_FORMAT_AAC:
            return "AAC";
        case CODEC_FORMAT_OGG_VORBIS:
            return "OGG Vorbis";
        case CODEC_FORMAT_PCM:
            return "PCM";
        case CODEC_FORMAT_H264:
            return "H.264";
        case CODEC_FORMAT_H265:
            return "H.265/HEVC";
        case CODEC_FORMAT_VP8:
            return "VP8";
        case CODEC_FORMAT_VP9:
            return "VP9";
        case CODEC_FORMAT_RAW:
            return "RAW";
        default:
            return "Unknown";
    }
}
