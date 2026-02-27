#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H

#include "types.h"
#include "alsa.h"

/* Audio formats */
#define AUDIO_FORMAT_S8      0x01
#define AUDIO_FORMAT_U8      0x02
#define AUDIO_FORMAT_S16_LE  0x03
#define AUDIO_FORMAT_S16_BE  0x04
#define AUDIO_FORMAT_S24_LE  0x05
#define AUDIO_FORMAT_S24_BE  0x06
#define AUDIO_FORMAT_S32_LE  0x07
#define AUDIO_FORMAT_S32_BE  0x08
#define AUDIO_FORMAT_FLOAT   0x09

/* Audio device */
typedef struct audio_device audio_device_t;

/* Audio stream */
typedef struct audio_stream audio_stream_t;

/* Initialize audio core */
void audio_core_init(void);

/* Create/destroy audio device */
audio_device_t* audio_device_create(const char* name, alsa_device_t* alsa_dev);
void audio_device_destroy(audio_device_t* dev);

/* Create/destroy audio stream */
audio_stream_t* audio_stream_create(audio_device_t* dev, u32 sample_rate,
                                    u32 channels, u32 format);
void audio_stream_destroy(audio_stream_t* stream);

/* Read/write audio data */
int audio_stream_write(audio_stream_t* stream, const void* data, size_t len);
int audio_stream_read(audio_stream_t* stream, void* data, size_t len);

/* Control audio stream */
int audio_stream_start(audio_stream_t* stream);
int audio_stream_stop(audio_stream_t* stream);

/* Audio mixer */
int audio_mixer_add_input(audio_stream_t* stream);
int audio_mixer_set_volume(u32 input_id, u32 volume);
int audio_mixer_mix(void* output_buffer, size_t len);

/* Get audio device properties */
u32 audio_get_sample_rate(audio_device_t* dev);
u32 audio_get_channels(audio_device_t* dev);

/* Find audio device */
audio_device_t* audio_find_device(const char* name);

#endif /* AUDIO_CORE_H */
