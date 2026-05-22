#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include "types.h"

/* Checkpoint image */
typedef struct checkpoint_image {
    char path[256];
    u64 timestamp;
    u64 size;
    void* data;
    struct checkpoint_image* next;
} checkpoint_image_t;

/* Initialize checkpoint/restore */
void checkpoint_init(void);

/* Create checkpoint */
int checkpoint_create(u64 pid, const char* path);

/* Restore from checkpoint */
int checkpoint_restore(const char* path);

/* List checkpoints */
int checkpoint_list(checkpoint_image_t** images, u32* count);

#endif /* CHECKPOINT_H */
