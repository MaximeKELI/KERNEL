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
    stream->name[sizeof(stream->name) - 1] = '\0'; /* Ensure null termination */
    stream->audio_stream = audio;
    stream->video_buffer = video;
    stream->audio_pts = 0;
    stream->video_pts = 0;
    stream->audio_duration = 0;
    stream->video_duration = 0;
    stream->start_time = hrtimer_get_time(); /* Use actual timestamp in nanoseconds */
    stream->playing = false;
    stream->paused = false;
    stream->destroyed = false;
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
    
    /* Protection against double-free */
    spinlock_lock(&stream->lock);
    if (stream->destroyed) {
        spinlock_unlock(&stream->lock);
        return;
    }
    stream->destroyed = true;
    spinlock_unlock(&stream->lock);
    
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
    stream->start_time = hrtimer_get_time(); /* Use actual timestamp in nanoseconds */
    
    int ret = 0;
    if (stream->audio_stream) {
        ret = audio_stream_start(stream->audio_stream);
        if (ret < 0) {
            /* If audio start fails, mark stream as not playing */
            stream->playing = false;
            spinlock_unlock(&stream->lock);
            DEBUG_ERROR("Failed to start audio stream: %s", stream->name);
            return -1;
        }
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
    
    u64 current_time = hrtimer_get_time(); /* Get current time in nanoseconds */
    u64 elapsed_ns = current_time - stream->start_time;
    u64 elapsed_ms = elapsed_ns / 1000000; /* Convert to milliseconds */
    
    /* Calculate drift between audio and video (PTS are in milliseconds) */
    i64 drift = (i64)stream->audio_pts - (i64)stream->video_pts;
    
    /* Also check against wall clock time */
    i64 audio_drift = (i64)stream->audio_pts - (i64)elapsed_ms;
    i64 video_drift = (i64)stream->video_pts - (i64)elapsed_ms;
    
    if (drift > SYNC_THRESHOLD_MS) {
        /* Audio ahead of video, slow down or skip video frame */
        DEBUG_INFO("AV sync: Audio ahead by %lld ms (audio=%llu, video=%llu)", 
                   (long long)drift, stream->audio_pts, stream->video_pts);
    } else if (drift < -SYNC_THRESHOLD_MS) {
        /* Video ahead of audio, slow down or skip audio */
        DEBUG_INFO("AV sync: Video ahead by %lld ms (audio=%llu, video=%llu)", 
                   (long long)-drift, stream->audio_pts, stream->video_pts);
    }
    
    /* Check for drift against wall clock */
    if (audio_drift > SYNC_THRESHOLD_MS || audio_drift < -SYNC_THRESHOLD_MS) {
        DEBUG_INFO("AV sync: Audio drift from wall clock: %lld ms", (long long)audio_drift);
    }
    if (video_drift > SYNC_THRESHOLD_MS || video_drift < -SYNC_THRESHOLD_MS) {
        DEBUG_INFO("AV sync: Video drift from wall clock: %lld ms", (long long)video_drift);
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
