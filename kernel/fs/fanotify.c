#include "fanotify.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "fs/vfs.h"
#include "validate.h"

#define MAX_FANOTIFY_INSTANCES 32
#define MAX_MARKS_PER_INSTANCE 512
#define FANOTIFY_EVENT_QUEUE_SIZE 2048

typedef struct fanotify_mark {
    u64 mask;
    char path[256];
    i32 dirfd;
    struct fanotify_mark* next;
} fanotify_mark_t;

typedef struct fanotify_instance {
    i32 fd;
    bool active;
    u32 flags;
    u32 event_f_flags;
    fanotify_mark_t* marks;
    fanotify_event_metadata_t* event_queue[FANOTIFY_EVENT_QUEUE_SIZE];
    u32 queue_head;
    u32 queue_tail;
    u32 queue_count;
    spinlock_t lock;
} fanotify_instance_t;

static fanotify_instance_t fanotify_instances[MAX_FANOTIFY_INSTANCES];
static i32 next_fd = 0;
static spinlock_t fanotify_global_lock = SPINLOCK_INIT;

static fanotify_instance_t* fanotify_get_instance(i32 fd) {
    for (u32 i = 0; i < MAX_FANOTIFY_INSTANCES; i++) {
        if (fanotify_instances[i].fd == fd && fanotify_instances[i].active) {
            return &fanotify_instances[i];
        }
    }
    return NULL;
}

int fanotify_init(u32 flags, u32 event_f_flags) {
    spinlock_lock(&fanotify_global_lock);
    
    for (u32 i = 0; i < MAX_FANOTIFY_INSTANCES; i++) {
        if (!fanotify_instances[i].active) {
            fanotify_instance_t* inst = &fanotify_instances[i];
            inst->fd = next_fd++;
            inst->active = true;
            inst->flags = flags;
            inst->event_f_flags = event_f_flags;
            inst->marks = NULL;
            inst->queue_head = 0;
            inst->queue_tail = 0;
            inst->queue_count = 0;
            spinlock_init(&inst->lock);
            
            spinlock_unlock(&fanotify_global_lock);
            DEBUG_INFO("Fanotify instance created: fd=%d, flags=0x%x", inst->fd, flags);
            return inst->fd;
        }
    }
    
    spinlock_unlock(&fanotify_global_lock);
    DEBUG_ERROR("%s", "Maximum fanotify instances reached");
    return -1;
}

int fanotify_mark(i32 fanotify_fd, u32 flags, u64 mask, i32 dirfd, const char* pathname) {
    VALIDATE_STRING(pathname, 256);
    
    fanotify_instance_t* inst = fanotify_get_instance(fanotify_fd);
    if (!inst) {
        DEBUG_ERROR("Invalid fanotify fd: %d", fanotify_fd);
        return -1;
    }
    
    spinlock_lock(&inst->lock);
    
    if (flags & FAN_MARK_REMOVE) {
        /* Remove mark */
        fanotify_mark_t** prev = &inst->marks;
        fanotify_mark_t* mark = inst->marks;
        
        while (mark) {
            if (mark->dirfd == dirfd && strcmp(mark->path, pathname) == 0) {
                *prev = mark->next;
                kfree(mark);
                spinlock_unlock(&inst->lock);
                DEBUG_INFO("Fanotify mark removed: path=%s", pathname);
                return 0;
            }
            prev = &mark->next;
            mark = mark->next;
        }
        
        spinlock_unlock(&inst->lock);
        return -1;
    } else {
        /* Add mark */
        u32 mark_count = 0;
        fanotify_mark_t* m = inst->marks;
        while (m) {
            mark_count++;
            m = m->next;
        }
        
        if (mark_count >= MAX_MARKS_PER_INSTANCE) {
            spinlock_unlock(&inst->lock);
            DEBUG_ERROR("%s", "Maximum marks per instance reached");
            return -1;
        }
        
        fanotify_mark_t* mark = (fanotify_mark_t*)kmalloc(sizeof(fanotify_mark_t));
        if (!mark) {
            spinlock_unlock(&inst->lock);
            return -1;
        }
        
        mark->mask = mask;
        mark->dirfd = dirfd;
        strncpy(mark->path, pathname, sizeof(mark->path) - 1);
        mark->path[sizeof(mark->path) - 1] = '\0';
        mark->next = inst->marks;
        inst->marks = mark;
        
        spinlock_unlock(&inst->lock);
        DEBUG_INFO("Fanotify mark added: path=%s, mask=0x%llx", pathname, (unsigned long long)mask);
        return 0;
    }
}

ssize_t fanotify_read(i32 fd, void* buf, size_t count) {
    VALIDATE_PTR_RET(buf, -1);
    
    fanotify_instance_t* inst = fanotify_get_instance(fd);
    if (!inst) {
        return -1;
    }
    
    spinlock_lock(&inst->lock);
    
    if (inst->queue_count == 0) {
        spinlock_unlock(&inst->lock);
        return 0;
    }
    
    fanotify_event_metadata_t* event = inst->event_queue[inst->queue_head];
    if (!event) {
        spinlock_unlock(&inst->lock);
        return 0;
    }
    
    if (count < event->metadata_len) {
        spinlock_unlock(&inst->lock);
        return -1;
    }
    
    memcpy(buf, event, event->metadata_len);
    kfree(event);
    
    inst->queue_head = (inst->queue_head + 1) % FANOTIFY_EVENT_QUEUE_SIZE;
    inst->queue_count--;
    
    spinlock_unlock(&inst->lock);
    return (ssize_t)event->metadata_len;
}

/* Internal function to queue event */
void fanotify_queue_event(i32 fd, u64 mask, const char* path) {
    fanotify_instance_t* inst = fanotify_get_instance(fd);
    if (!inst) return;
    
    spinlock_lock(&inst->lock);
    
    /* Check if any mark matches */
    fanotify_mark_t* mark = inst->marks;
    bool matched = false;
    while (mark) {
        if (strcmp(mark->path, path) == 0 && (mark->mask & mask)) {
            matched = true;
            break;
        }
        mark = mark->next;
    }
    
    if (matched && inst->queue_count < FANOTIFY_EVENT_QUEUE_SIZE) {
        fanotify_event_metadata_t* event = (fanotify_event_metadata_t*)kmalloc(sizeof(fanotify_event_metadata_t));
        if (event) {
            event->event_len = sizeof(fanotify_event_metadata_t);
            event->vers = 3;
            event->reserved = 0;
            event->metadata_len = sizeof(fanotify_event_metadata_t);
            event->mask = mask;
            event->fd = -1; /* Would be set to actual file descriptor */
            event->pid = 0; /* Would be set to process ID */
            
            u32 tail = (inst->queue_tail + 1) % FANOTIFY_EVENT_QUEUE_SIZE;
            inst->event_queue[inst->queue_tail] = event;
            inst->queue_tail = tail;
            inst->queue_count++;
        }
    }
    
    spinlock_unlock(&inst->lock);
}
