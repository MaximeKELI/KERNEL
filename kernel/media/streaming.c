#include "streaming.h"
#include "codec.h"
#include "audio_core.h"
#include "video_core.h"
#include "av_sync.h"
#include "net.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"
#include "validate.h"

#define MAX_STREAMS 32
#define STREAM_BUFFER_SIZE (512 * 1024)  /* 512KB */

/* Media stream */
typedef struct media_stream {
    u32 stream_id;
    char name[32];
    stream_protocol_t protocol;
    stream_state_t state;
    socket_t* socket;
    codec_t* audio_codec;
    codec_t* video_codec;
    audio_stream_t* audio_stream;
    video_buffer_t* video_buffer;
    av_stream_t* av_stream;
    void* buffer;
    size_t buffer_size;
    size_t buffer_fill;
    u64 bytes_received;
    u64 bytes_sent;
    bool active;
    bool destroyed;  /* Protection against double-free */
    spinlock_t lock;
    struct media_stream* next;
} media_stream_t;

static media_stream_t* streams = NULL;
static spinlock_t stream_lock = SPINLOCK_INIT;
static u32 stream_counter = 0;

void streaming_init(void) {
    streams = NULL;
    stream_counter = 0;
    printk("[Streaming] Media streaming subsystem initialized\n");
}

media_stream_t* stream_create(const char* name, stream_protocol_t protocol) {
    VALIDATE_PTR_RET(name, NULL);
    
    media_stream_t* stream = (media_stream_t*)kzalloc(sizeof(media_stream_t));
    if (!stream) {
        return NULL;
    }
    
    stream->stream_id = stream_counter++;
    strncpy(stream->name, name, sizeof(stream->name) - 1);
    stream->name[sizeof(stream->name) - 1] = '\0'; /* Ensure null termination */
    stream->protocol = protocol;
    stream->state = STREAM_STATE_IDLE;
    stream->socket = NULL;
    stream->audio_codec = NULL;
    stream->video_codec = NULL;
    stream->audio_stream = NULL;
    stream->video_buffer = NULL;
    stream->av_stream = NULL;
    stream->buffer_size = STREAM_BUFFER_SIZE;
    stream->buffer = kzalloc(stream->buffer_size);
    if (!stream->buffer) {
        kfree(stream);
        return NULL;
    }
    stream->buffer_fill = 0;
    stream->bytes_received = 0;
    stream->bytes_sent = 0;
    stream->active = false;
    spinlock_init(&stream->lock);
    
    spinlock_lock(&stream_lock);
    stream->next = streams;
    streams = stream;
    spinlock_unlock(&stream_lock);
    
    DEBUG_INFO("Media stream created: %s, protocol=%u", name, protocol);
    return stream;
}

void stream_destroy(media_stream_t* stream) {
    VALIDATE_PTR_VOID(stream);
    
    /* Stop stream */
    stream_stop(stream);
    
    spinlock_lock(&stream_lock);
    
    /* Remove from list */
    if (streams == stream) {
        streams = stream->next;
    } else {
        media_stream_t* current = streams;
        while (current && current->next != stream) {
            current = current->next;
        }
        if (current) {
            current->next = stream->next;
        }
    }
    
    spinlock_unlock(&stream_lock);
    
    /* Cleanup */
    if (stream->socket) {
        socket_destroy(stream->socket);
    }
    
    if (stream->audio_codec) {
        codec_destroy(stream->audio_codec);
    }
    
    if (stream->video_codec) {
        codec_destroy(stream->video_codec);
    }
    
    if (stream->av_stream) {
        av_stream_destroy(stream->av_stream);
    }
    
    if (stream->buffer) {
        kfree(stream->buffer);
    }
    
    kfree(stream);
}

/* Helper function to parse IP address string to ip_addr_t */
static int parse_ip_address(const char* host, ip_addr_t* addr) {
    if (!host || !addr) {
        return -1;
    }
    
    /* Simple IP address parsing (supports IPv4 dotted decimal) */
    u8 ip[4] = {0};
    int parts[4] = {0};
    int num_parts = 0;
    const char* p = host;
    
    /* Parse dotted decimal format: a.b.c.d */
    while (*p && num_parts < 4) {
        if (*p >= '0' && *p <= '9') {
            parts[num_parts] = parts[num_parts] * 10 + (*p - '0');
            if (parts[num_parts] > 255) {
                return -1; /* Invalid IP */
            }
        } else if (*p == '.') {
            num_parts++;
        } else {
            return -1; /* Invalid character */
        }
        p++;
    }
    num_parts++; /* Count last part */
    
    if (num_parts != 4) {
        return -1; /* Invalid IP format */
    }
    
    for (int i = 0; i < 4; i++) {
        ip[i] = (u8)parts[i];
    }
    
    addr->addr[0] = ip[0];
    addr->addr[1] = ip[1];
    addr->addr[2] = ip[2];
    addr->addr[3] = ip[3];
    
    return 0;
}

/* Helper function to create sockaddr_t from IP and port */
static void create_sockaddr(ip_addr_t* ip, u16 port, sockaddr_t* addr) {
    if (!ip || !addr) {
        return;
    }
    
    addr->sa_family = 2; /* AF_INET */
    /* Format: port (2 bytes, network byte order) + IP (4 bytes) */
    addr->sa_data[0] = (port >> 8) & 0xFF;
    addr->sa_data[1] = port & 0xFF;
    addr->sa_data[2] = ip->addr[0];
    addr->sa_data[3] = ip->addr[1];
    addr->sa_data[4] = ip->addr[2];
    addr->sa_data[5] = ip->addr[3];
    /* Rest is zero */
    for (int i = 6; i < 14; i++) {
        addr->sa_data[i] = 0;
    }
}

int stream_connect(media_stream_t* stream, const char* host, u16 port) {
    VALIDATE_PTR_RET(stream, -1);
    VALIDATE_PTR_RET(host, -1);
    
    /* Create socket based on protocol */
    int domain = 2; /* AF_INET */
    int type = (stream->protocol == STREAM_PROTOCOL_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int protocol = (stream->protocol == STREAM_PROTOCOL_UDP) ? IPPROTO_UDP : IPPROTO_TCP;
    
    stream->socket = socket_create(domain, type, protocol);
    if (!stream->socket) {
        DEBUG_ERROR("Failed to create socket for stream: %s", stream->name);
        return -1;
    }
    
    /* Parse host IP address */
    ip_addr_t remote_ip = {0};
    if (parse_ip_address(host, &remote_ip) < 0) {
        DEBUG_ERROR("Invalid IP address: %s", host);
        socket_destroy(stream->socket);
        stream->socket = NULL;
        return -1;
    }
    
    /* Create sockaddr for connection */
    sockaddr_t addr;
    create_sockaddr(&remote_ip, port, &addr);
    
    /* Connect socket */
    if (stream->socket->ops && stream->socket->ops->connect) {
        int ret = stream->socket->ops->connect(stream->socket, &addr);
        if (ret < 0) {
            DEBUG_ERROR("Failed to connect socket for stream: %s", stream->name);
            socket_destroy(stream->socket);
            stream->socket = NULL;
            return -1;
        }
    } else {
        DEBUG_ERROR("Socket connect operation not available", 0);
        socket_destroy(stream->socket);
        stream->socket = NULL;
        return -1;
    }
    
    stream->state = STREAM_STATE_CONNECTING;
    
    DEBUG_INFO("Stream connecting: %s -> %s:%u", stream->name, host, port);
    return 0;
}

int stream_start(media_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    
    if (stream->state != STREAM_STATE_CONNECTING && 
        stream->state != STREAM_STATE_BUFFERING) {
        spinlock_unlock(&stream->lock);
        return -1;
    }
    
    stream->state = STREAM_STATE_PLAYING;
    stream->active = true;
    
    if (stream->av_stream) {
        av_stream_play(stream->av_stream);
    }
    
    spinlock_unlock(&stream->lock);
    
    DEBUG_INFO("Stream started: %s", stream->name);
    return 0;
}

int stream_stop(media_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    
    stream->state = STREAM_STATE_IDLE;
    stream->active = false;
    
    if (stream->av_stream) {
        av_stream_stop(stream->av_stream);
    }
    
    spinlock_unlock(&stream->lock);
    
    DEBUG_INFO("Stream stopped: %s", stream->name);
    return 0;
}

int stream_pause(media_stream_t* stream) {
    VALIDATE_PTR_RET(stream, -1);
    
    spinlock_lock(&stream->lock);
    
    if (stream->state != STREAM_STATE_PLAYING) {
        spinlock_unlock(&stream->lock);
        return -1;
    }
    
    stream->state = STREAM_STATE_PAUSED;
    
    if (stream->av_stream) {
        av_stream_pause(stream->av_stream);
    }
    
    spinlock_unlock(&stream->lock);
    
    DEBUG_INFO("Stream paused: %s", stream->name);
    return 0;
}

int stream_receive_data(media_stream_t* stream, const void* data, size_t len) {
    VALIDATE_PTR_RET(stream, -1);
    VALIDATE_PTR_RET(data, -1);
    VALIDATE_SIZE(len);
    
    if (!stream->active) {
        return -1;
    }
    
    spinlock_lock(&stream->lock);
    
    /* Add to buffer */
    size_t free_space = stream->buffer_size - stream->buffer_fill;
    if (len > free_space) {
        len = free_space; /* Drop excess */
    }
    
    if (len > 0) {
        memcpy((u8*)stream->buffer + stream->buffer_fill, data, len);
        stream->buffer_fill += len;
        stream->bytes_received += len;
    }
    
    /* Process buffer if enough data */
    if (stream->buffer_fill >= 4096) { /* Process in 4KB chunks */
        /* TODO: Parse and decode stream data */
        /* For now, just consume */
        stream->buffer_fill = 0;
    }
    
    spinlock_unlock(&stream->lock);
    
    return len;
}

media_stream_t* stream_find(const char* name) {
    if (!name) {
        return NULL;
    }
    
    spinlock_lock(&stream_lock);
    
    media_stream_t* stream = streams;
    while (stream) {
        if (strcmp(stream->name, name) == 0) {
            spinlock_unlock(&stream_lock);
            return stream;
        }
        stream = stream->next;
    }
    
    spinlock_unlock(&stream_lock);
    return NULL;
}

u64 stream_get_bytes_received(media_stream_t* stream) {
    VALIDATE_PTR_RET(stream, 0);
    return stream->bytes_received;
}

u64 stream_get_bytes_sent(media_stream_t* stream) {
    VALIDATE_PTR_RET(stream, 0);
    return stream->bytes_sent;
}
