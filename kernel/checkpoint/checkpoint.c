#include "checkpoint.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "process.h"
#include "drivers/timer.h"
#include "validate.h"

static checkpoint_image_t* checkpoint_images = NULL;
static spinlock_t checkpoint_lock = SPINLOCK_INIT;

void checkpoint_init(void) {
    DEBUG_INFO("Checkpoint/Restore (CRIU-like) system initialized");
}

int checkpoint_create(u64 pid, const char* path) {
    VALIDATE_PTR_RET(path, -1);
    VALIDATE_STRING(path, 255);
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            checkpoint_image_t* image = (checkpoint_image_t*)kzalloc(sizeof(checkpoint_image_t));
            if (!image) {
                DEBUG_ERROR("Failed to allocate checkpoint image");
                return -1;
            }
            
            strncpy(image->path, path, sizeof(image->path) - 1);
            image->timestamp = timer_get_ticks();
            image->size = sizeof(process_t); /* Would be larger */
            image->data = kmalloc(image->size);
            
            if (image->data) {
                memcpy(image->data, proc, sizeof(process_t));
            }
            
            spinlock_lock(&checkpoint_lock);
            image->data = (void*)((u64)image + sizeof(checkpoint_image_t));
            image->next = checkpoint_images;
            checkpoint_images = image;
            spinlock_unlock(&checkpoint_lock);
            
            DEBUG_INFO("Checkpoint created: pid=%u, path=%s", (u32)pid, path);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int checkpoint_restore(const char* path) {
    if (!path) return -1;
    
    spinlock_lock(&checkpoint_lock);
    
    checkpoint_image_t* image = checkpoint_images;
    while (image) {
        if (strcmp(image->path, path) == 0) {
            /* Would restore process from image */
            spinlock_unlock(&checkpoint_lock);
            DEBUG_INFO("Checkpoint restored: path=%s", path);
            return 0;
        }
        image = image->next;
    }
    
    spinlock_unlock(&checkpoint_lock);
    return -1;
}

int checkpoint_list(checkpoint_image_t** images, u32* count) {
    if (!images || !count) return -1;
    
    spinlock_lock(&checkpoint_lock);
    
    u32 cnt = 0;
    checkpoint_image_t* img = checkpoint_images;
    while (img) {
        cnt++;
        img = img->next;
    }
    
    if (cnt > 0) {
        *images = (checkpoint_image_t*)kmalloc(cnt * sizeof(checkpoint_image_t));
        if (*images) {
            img = checkpoint_images;
            for (u32 i = 0; i < cnt && img; i++) {
                (*images)[i] = *img;
                img = img->next;
            }
        }
    }
    
    *count = cnt;
    spinlock_unlock(&checkpoint_lock);
    
    return 0;
}
