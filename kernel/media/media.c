#include "media.h"
#include "audio_core.h"
#include "video_core.h"
#include "codec.h"
#include "av_sync.h"
#include "streaming.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"

void media_init(void) {
    /* Initialize all media subsystems */
    audio_core_init();
    video_core_init();
    codec_init();
    av_sync_init();
    streaming_init();
    
    DEBUG_INFO("Media subsystem initialized");
    printk("[Media] Complete multimedia subsystem initialized\n");
}
