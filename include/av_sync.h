#ifndef AV_SYNC_H
#define AV_SYNC_H

#include "types.h"
#include "audio_core.h"
#include "video_core.h"

/* AV stream */
typedef struct av_stream av_stream_t;

/* Initialize AV sync */
void av_sync_init(void);

/* Create/destroy AV stream */
av_stream_t* av_stream_create(const char* name, audio_stream_t* audio,
                              video_buffer_t* video);
void av_stream_destroy(av_stream_t* stream);

/* Control AV stream */
int av_stream_play(av_stream_t* stream);
int av_stream_pause(av_stream_t* stream);
int av_stream_stop(av_stream_t* stream);

/* Set presentation timestamps */
int av_stream_set_pts(av_stream_t* stream, u64 audio_pts, u64 video_pts);

/* Synchronize audio and video */
int av_stream_sync(av_stream_t* stream);

/* Find AV stream */
av_stream_t* av_stream_find(const char* name);

#endif /* AV_SYNC_H */
