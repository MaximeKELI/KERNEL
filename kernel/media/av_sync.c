#include "av_sync.h"
#include "audio_core.h"
#include "video_core.h"
#include "timer_hr.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "validate.h"

#define MAX_AV_STREAMS 16
#define SYNC_THRESHOLD_MS 40  /* 40ms tolerance */

/* AV stream */
typedef struct av_stream {
    u32 stream_id;
    char name[32];
    audio_stream_t* audio_stream;
    video_buffer_t* video_buffer;
    u64 audio_pts;  /* Presentation timestamp */
    u64 video_pts;
    u64 audio_duration;
    u64 video_duration;
    u64 start_time;
    bool playing;
    bool paused;
    spinlock_t lock;
    struct av_stream* next;
} av_stream_t;

static av_stream_t* av_streams = NULL;
static spinlock_t av_global_lock = SPINLOCK_INIT;
static u32 av_stream_counter = 0;

void av_sync_init(void) {
    av_streams = NULL;
    av_stream_counter = 0;
    printk("[AV Sync] Audio/Video synchronization subsystem initialized\n");
}

av_stream_t* av_stream_create(const char* name, audio_stream_t* audio,
                              video_buffer_t* video) {
    VALIDATE_PTR_RET(name, NULL);
    
    av_stream_t* stream = (av_stream_t*)kzalloc(sizeof(av_stream_t));
    if (!stream) {
        return NULL;
    }
    
    stream->stream_id = av_stream_counter++;
    strncpy(stream->name, name, sizeof(stream->name) - 1);
    stream->audio_stream = audio;
    stream->video_buffer = video;
    stream->audio_pts = 0;
    stream->video_pts = 0;
    stream->audio_duration = 0;
    stream->video_duration = 0;
    stream->start_time = 0; /* TODO: Use actual timestamp */
    stream->playing = false;
    stream->paused = false;
    spinlock_init(&stream->lock);
    
    spinlock_lock(&av_global_lock);
    stream->next = av_streams;
    av_streams = stream;
    spinlock_unlock(&av_global_lock);
    
    DEBUG_INFO("AV stream created: %s", name);
    return stream;
}

void av_stream_destroy(av_stream_t* stream) {
    VALIDATE_PTR_VOID(stream);
    
    spinlock_lock(&av_global_lock);
    
    /* Remove from list */
    if (av_streams == stream) {
        av_streams = stream->next;
    } else {
        av_stream_t* current = av_streams;
        while (current && current->next != stream) {
            current = current->next;
        }
        if (current) {
            current->next = stream->next;
        }
    }
    
    spinlock_unlock(&av_global_lock);
    
    kfree(stream);
}

int av_stream_play(av_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    
    if (stream->playing) {
        spinlock_unlock(&stream->lock);
        return 0; /* Already playing */
    }
    
    stream->playing = true;
    stream->paused = false;
    stream->start_time = 0; /* TODO: Use actual timestamp */
    
    if (stream->audio_stream) {
        audio_stream_start(stream->audio_stream);
    }
    
    spinlock_unlock(&stream->lock);
    
    DEBUG_INFO("AV stream playing: %s", stream->name);
    return 0;
}

int av_stream_pause(av_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    
    if (!stream->playing) {
        spinlock_unlock(&stream->lock);
        return -1;
    }
    
    stream->paused = true;
    
    if (stream->audio_stream) {
        audio_stream_stop(stream->audio_stream);
    }
    
    spinlock_unlock(&stream->lock);
    
    DEBUG_INFO("AV stream paused: %s", stream->name);
    return 0;
}

int av_stream_stop(av_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    
    stream->playing = false;
    stream->paused = false;
    stream->audio_pts = 0;
    stream->video_pts = 0;
    
    if (stream->audio_stream) {
        audio_stream_stop(stream->audio_stream);
    }
    
    spinlock_unlock(&stream->lock);
    
    DEBUG_INFO("AV stream stopped: %s", stream->name);
    return 0;
}

int av_stream_set_pts(av_stream_t* stream, u64 audio_pts, u64 video_pts) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    stream->audio_pts = audio_pts;
    stream->video_pts = video_pts;
    spinlock_unlock(&stream->lock);
    
    return 0;
}

int av_stream_sync(av_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    if (!stream->playing || stream->paused) {
        return 0;
    }
    
    spinlock_lock(&stream->lock);
    
    u64 current_time = 0; /* TODO: Use actual timestamp */
    u64 elapsed = current_time - stream->start_time;
    
    /* Calculate drift between audio and video */
    s64 drift = (s64)stream->audio_pts - (s64)stream->video_pts;
    
    if (drift > SYNC_THRESHOLD_MS) {
        /* Audio ahead, slow down or skip video frame */
        DEBUG_INFO("AV sync: Audio ahead by %lld ms", drift);
    } else if (drift < -SYNC_THRESHOLD_MS) {
        /* Video ahead, slow down or skip audio */
        DEBUG_INFO("AV sync: Video ahead by %lld ms", -drift);
    }
    
    spinlock_unlock(&stream->lock);
    
    return 0;
}

av_stream_t* av_stream_find(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&av_global_lock);
    
    av_stream_t* stream = av_streams;
    while (stream) {
        if (strcmp(stream->name, name) == 0) {
            spinlock_unlock(&av_global_lock);
            return stream;
        }
        stream = stream->next;
    }
    
    spinlock_unlock(&av_global_lock);
    return NULL;
}
