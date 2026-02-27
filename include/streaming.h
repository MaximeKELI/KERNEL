#ifndef STREAMING_H
#define STREAMING_H

#include "types.h"
#include "net.h"

/* Stream protocol */
typedef enum {
    STREAM_PROTOCOL_RTP,
    STREAM_PROTOCOL_RTSP,
    STREAM_PROTOCOL_HTTP,
    STREAM_PROTOCOL_UDP,
    STREAM_PROTOCOL_TCP
} stream_protocol_t;

/* Stream state */
typedef enum {
    STREAM_STATE_IDLE,
    STREAM_STATE_CONNECTING,
    STREAM_STATE_BUFFERING,
    STREAM_STATE_PLAYING,
    STREAM_STATE_PAUSED,
    STREAM_STATE_ERROR
} stream_state_t;

/* Media stream */
typedef struct media_stream media_stream_t;

/* Initialize streaming */
void streaming_init(void);

/* Create/destroy stream */
media_stream_t* stream_create(const char* name, stream_protocol_t protocol);
void stream_destroy(media_stream_t* stream);

/* Connect stream */
int stream_connect(media_stream_t* stream, const char* host, u16 port);

/* Control stream */
int stream_start(media_stream_t* stream);
int stream_stop(media_stream_t* stream);
int stream_pause(media_stream_t* stream);

/* Receive stream data */
int stream_receive_data(media_stream_t* stream, const void* data, size_t len);

/* Get stream statistics */
u64 stream_get_bytes_received(media_stream_t* stream);
u64 stream_get_bytes_sent(media_stream_t* stream);

/* Find stream */
media_stream_t* stream_find(const char* name);

#endif /* STREAMING_H */
