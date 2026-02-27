#include "audio_core.h"
#include "alsa.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "validate.h"

#define MAX_AUDIO_DEVICES 16
#define MAX_AUDIO_STREAMS 64
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_CHANNELS 2
#define DEFAULT_FORMAT AUDIO_FORMAT_S16_LE
#define AUDIO_BUFFER_SIZE (64 * 1024)  /* 64KB */

/* Audio stream */
typedef struct audio_stream {
    u32 stream_id;
    u32 sample_rate;
    u32 channels;
    u32 format;
    u32 buffer_size;
    void* buffer;
    u32 write_pos;
    u32 read_pos;
    u32 available;
    bool active;
    spinlock_t lock;
    struct audio_stream* next;
} audio_stream_t;

/* Audio device */
typedef struct audio_device {
    u32 device_id;
    char name[32];
    alsa_device_t* alsa_dev;
    audio_stream_t* streams[MAX_AUDIO_STREAMS];
    u32 num_streams;
    u32 sample_rate;
    u32 channels;
    u32 format;
    bool active;
    struct audio_device* next;
} audio_device_t;

/* Audio mixer */
typedef struct audio_mixer {
    u32 num_inputs;
    u32 num_outputs;
    audio_stream_t* inputs[MAX_AUDIO_STREAMS];
    audio_stream_t* outputs[MAX_AUDIO_STREAMS];
    u32 volumes[MAX_AUDIO_STREAMS];
    spinlock_t lock;
} audio_mixer_t;

static audio_device_t* audio_devices = NULL;
static audio_mixer_t global_mixer = {0};
static spinlock_t audio_global_lock = SPINLOCK_INIT;
static u32 audio_stream_counter = 0;

void audio_core_init(void) {
    audio_devices = NULL;
    audio_stream_counter = 0;
    global_mixer.num_inputs = 0;
    global_mixer.num_outputs = 0;
    spinlock_init(&global_mixer.lock);
    
    printk("[Audio Core] Audio core subsystem initialized\n");
}

audio_device_t* audio_device_create(const char* name, alsa_device_t* alsa_dev) {
    VALIDATE_PTR_RET(name, NULL);
    VALIDATE_PTR_RET(alsa_dev, NULL);
    
    audio_device_t* dev = (audio_device_t*)kzalloc(sizeof(audio_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->device_id = 0; /* TODO: Generate unique ID */
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->alsa_dev = alsa_dev;
    dev->num_streams = 0;
    dev->sample_rate = DEFAULT_SAMPLE_RATE;
    dev->channels = DEFAULT_CHANNELS;
    dev->format = DEFAULT_FORMAT;
    dev->active = false;
    
    spinlock_lock(&audio_global_lock);
    dev->next = audio_devices;
    audio_devices = dev;
    spinlock_unlock(&audio_global_lock);
    
    DEBUG_INFO("Audio device created: %s", name);
    return dev;
}

audio_stream_t* audio_stream_create(audio_device_t* dev, u32 sample_rate, 
                                    u32 channels, u32 format) {
    VALIDATE_PTR_RET(dev, NULL);
    VALIDATE_RANGE(sample_rate, 8000, 192000);
    VALIDATE_RANGE(channels, 1, 8);
    
    spinlock_lock(&audio_global_lock);
    
    /* Check limit while holding lock to avoid race condition */
    if (dev->num_streams >= MAX_AUDIO_STREAMS) {
        spinlock_unlock(&audio_global_lock);
        return NULL;
    }
    
    spinlock_unlock(&audio_global_lock);
    
    audio_stream_t* stream = (audio_stream_t*)kzalloc(sizeof(audio_stream_t));
    if (!stream) {
        return NULL;
    }
    
    /* Initialize all fields BEFORE adding to list (fixes race condition) */
    stream->stream_id = audio_stream_counter++;
    stream->sample_rate = sample_rate;
    stream->channels = channels;
    stream->format = format;
    stream->buffer_size = AUDIO_BUFFER_SIZE;
    stream->buffer = kzalloc(stream->buffer_size);
    if (!stream->buffer) {
        kfree(stream);
        return NULL;
    }
    
    stream->write_pos = 0;
    stream->read_pos = 0;
    stream->available = 0;
    stream->active = false;
    stream->next = NULL;
    spinlock_init(&stream->lock);
    
    /* Now add to list - stream is fully initialized */
    spinlock_lock(&audio_global_lock);
    
    /* Double-check limit (may have changed) */
    if (dev->num_streams >= MAX_AUDIO_STREAMS) {
        spinlock_unlock(&audio_global_lock);
        kfree(stream->buffer);
        kfree(stream);
        return NULL;
    }
    
    dev->streams[dev->num_streams++] = stream;
    spinlock_unlock(&audio_global_lock);
    
    DEBUG_INFO("Audio stream created: id=%u, rate=%u, ch=%u", 
              stream->stream_id, sample_rate, channels);
    return stream;
}

int audio_stream_write(audio_stream_t* stream, const void* data, size_t len) {
    VALIDATE_PTR_RET(stream, -1);
    VALIDATE_PTR_RET(data, -1);
    VALIDATE_SIZE(len);
    
    if (!stream->active) {
        return -1;
    }
    
    spinlock_lock(&stream->lock);
    
    size_t free_space = stream->buffer_size - stream->available;
    if (len > free_space) {
        len = free_space; /* Drop excess data */
    }
    
    if (len > 0) {
        u8* buf = (u8*)stream->buffer;
        size_t to_end = stream->buffer_size - stream->write_pos;
        
        if (len <= to_end) {
            memcpy(buf + stream->write_pos, data, len);
            stream->write_pos = (stream->write_pos + len) % stream->buffer_size;
        } else {
            memcpy(buf + stream->write_pos, data, to_end);
            memcpy(buf, (u8*)data + to_end, len - to_end);
            stream->write_pos = len - to_end;
        }
        
        stream->available += len;
    }
    
    spinlock_unlock(&stream->lock);
    
    return len;
}

int audio_stream_read(audio_stream_t* stream, void* data, size_t len) {
    VALIDATE_PTR_RET(stream, -1);
    VALIDATE_PTR_RET(data, -1);
    VALIDATE_SIZE(len);
    
    if (!stream->active) {
        return -1;
    }
    
    spinlock_lock(&stream->lock);
    
    if (len > stream->available) {
        len = stream->available;
    }
    
    if (len > 0) {
        u8* buf = (u8*)stream->buffer;
        size_t to_end = stream->buffer_size - stream->read_pos;
        
        if (len <= to_end) {
            memcpy(data, buf + stream->read_pos, len);
            stream->read_pos = (stream->read_pos + len) % stream->buffer_size;
        } else {
            memcpy(data, buf + stream->read_pos, to_end);
            memcpy((u8*)data + to_end, buf, len - to_end);
            stream->read_pos = len - to_end;
        }
        
        stream->available -= len;
    }
    
    spinlock_unlock(&stream->lock);
    
    return len;
}

int audio_stream_start(audio_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    stream->active = true;
    DEBUG_INFO("Audio stream started: id=%u", stream->stream_id);
    return 0;
}

int audio_stream_stop(audio_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    stream->active = false;
    DEBUG_INFO("Audio stream stopped: id=%u", stream->stream_id);
    return 0;
}

int audio_mixer_add_input(audio_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&global_mixer.lock);
    
    if (global_mixer.num_inputs >= MAX_AUDIO_STREAMS) {
        spinlock_unlock(&global_mixer.lock);
        return -1;
    }
    
    global_mixer.inputs[global_mixer.num_inputs] = stream;
    global_mixer.volumes[global_mixer.num_inputs] = 100; /* 100% */
    global_mixer.num_inputs++;
    
    spinlock_unlock(&global_mixer.lock);
    
    return 0;
}

int audio_mixer_set_volume(u32 input_id, u32 volume) {
    VALIDATE_RANGE(volume, 0, 100);
    
    spinlock_lock(&global_mixer.lock);
    
    if (input_id >= global_mixer.num_inputs) {
        spinlock_unlock(&global_mixer.lock);
        return -1;
    }
    
    global_mixer.volumes[input_id] = volume;
    
    spinlock_unlock(&global_mixer.lock);
    
    return 0;
}

int audio_mixer_mix(void* output_buffer, size_t len) {
    VALIDATE_PTR_RET(output_buffer, -1);
    VALIDATE_SIZE(len);
    
    memset(output_buffer, 0, len);
    
    spinlock_lock(&global_mixer.lock);
    
    /* Mix all active input streams */
    for (u32 i = 0; i < global_mixer.num_inputs; i++) {
        audio_stream_t* stream = global_mixer.inputs[i];
        if (!stream || !stream->active) {
            continue;
        }
        
        u32 volume = global_mixer.volumes[i];
        size_t to_read = len;
        if (to_read > stream->available) {
            to_read = stream->available;
        }
        
        if (to_read > 0) {
            u8 temp_buffer[4096];
            size_t read = audio_stream_read(stream, temp_buffer, to_read);
            
            if (read > 0) {
                /* Simple mixing: add samples with volume */
                i16* out_samples = (i16*)output_buffer;
                i16* in_samples = (i16*)temp_buffer;
                size_t num_samples = read / sizeof(i16);
                
                for (size_t j = 0; j < num_samples; j++) {
                    i32 mixed = (i32)out_samples[j] + 
                               ((i32)in_samples[j] * volume / 100);
                    if (mixed > 32767) mixed = 32767;
                    if (mixed < -32768) mixed = -32768;
                    out_samples[j] = (i16)mixed;
                }
            }
        }
    }
    
    spinlock_unlock(&global_mixer.lock);
    
    return len;
}

u32 audio_get_sample_rate(audio_device_t* dev) {
    VALIDATE_PTR_RET(dev, 0);
    return dev->sample_rate;
}

u32 audio_get_channels(audio_device_t* dev) {
    VALIDATE_PTR_RET(dev, 0);
    return dev->channels;
}

audio_device_t* audio_find_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&audio_global_lock);
    
    audio_device_t* dev = audio_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            spinlock_unlock(&audio_global_lock);
            return dev;
        }
        dev = dev->next;
    }
    
    spinlock_unlock(&audio_global_lock);
    return NULL;
}
