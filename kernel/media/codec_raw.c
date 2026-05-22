#include "stdio.h"
#include "memory.h"
#include "string.h"

typedef struct raw_pcm_desc {
    u32 sample_rate;
    u16 channels;
    u16 bits;
} raw_pcm_desc_t;

int codec_raw_decode(const u8* in, size_t in_len, u8* out, size_t out_cap, size_t* out_len) {
    if (!in || !out || !out_len) {
        return -1;
    }
    size_t n = in_len;
    if (n > out_cap) {
        n = out_cap;
    }
    memcpy(out, in, n);
    *out_len = n;
    return 0;
}

void codec_raw_init(void) {
    printk("[codec] RAW PCM passthrough ready\n");
}
