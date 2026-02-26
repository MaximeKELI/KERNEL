#include "inotify.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "fs/vfs.h"
#include "validate.h"

#define MAX_INOTIFY_INSTANCES 64
#define MAX_WATCHES_PER_INSTANCE 256
#define INOTIFY_EVENT_QUEUE_SIZE 1024

typedef struct inotify_instance {
    i32 fd;
    bool active;
    inotify_watch_t* watches;
    inotify_event_t* event_queue[INOTIFY_EVENT_QUEUE_SIZE];
    u32 queue_head;
    u32 queue_tail;
    u32 queue_count;
    spinlock_t lock;
    i32 next_wd;
} inotify_instance_t;

static inotify_instance_t inotify_instances[MAX_INOTIFY_INSTANCES];
static i32 next_fd = 0;
static spinlock_t inotify_global_lock = SPINLOCK_INIT;

static inotify_instance_t* inotify_get_instance(i32 fd) {
    for (u32 i = 0; i < MAX_INOTIFY_INSTANCES; i++) {
        if (inotify_instances[i].fd == fd && inotify_instances[i].active) {
            return &inotify_instances[i];
        }
    }
    return NULL;
}

int inotify_init(void) {
    spinlock_lock(&inotify_global_lock);
    
    for (u32 i = 0; i < MAX_INOTIFY_INSTANCES; i++) {
        if (!inotify_instances[i].active) {
            inotify_instance_t* inst = &inotify_instances[i];
            inst->fd = next_fd++;
            inst->active = true;
            inst->watches = NULL;
            inst->queue_head = 0;
            inst->queue_tail = 0;
            inst->queue_count = 0;
            inst->next_wd = 1;
            spinlock_init(&inst->lock);
            
            spinlock_unlock(&inotify_global_lock);
            DEBUG_INFO("Inotify instance created: fd=%d", inst->fd);
            return inst->fd;
        }
    }
    
    spinlock_unlock(&inotify_global_lock);
    DEBUG_ERROR("%s", "Maximum inotify instances reached");
    return -1;
}

i32 inotify_add_watch(i32 fd, const char* pathname, u32 mask) {
    VALIDATE_STRING(pathname, 256);
    
    inotify_instance_t* inst = inotify_get_instance(fd);
    if (!inst) {
        DEBUG_ERROR("Invalid inotify fd: %d", fd);
        return -1;
    }
    
    spinlock_lock(&inst->lock);
    
    /* Count existing watches */
    u32 watch_count = 0;
    inotify_watch_t* w = inst->watches;
    while (w) {
        watch_count++;
        w = w->next;
    }
    
    if (watch_count >= MAX_WATCHES_PER_INSTANCE) {
        spinlock_unlock(&inst->lock);
        DEBUG_ERROR("%s", "Maximum watches per instance reached");
        return -1;
    }
    
    /* Create new watch */
    inotify_watch_t* watch = (inotify_watch_t*)kmalloc(sizeof(inotify_watch_t));
    if (!watch) {
        spinlock_unlock(&inst->lock);
        return -1;
    }
    
    watch->wd = inst->next_wd++;
    watch->mask = mask;
    strncpy(watch->path, pathname, sizeof(watch->path) - 1);
    watch->path[sizeof(watch->path) - 1] = '\0';
    watch->next = inst->watches;
    inst->watches = watch;
    
    spinlock_unlock(&inst->lock);
    
    DEBUG_INFO("Inotify watch added: wd=%d, path=%s, mask=0x%x", watch->wd, pathname, mask);
    return watch->wd;
}

int inotify_rm_watch(i32 fd, i32 wd) {
    inotify_instance_t* inst = inotify_get_instance(fd);
    if (!inst) {
        return -1;
    }
    
    spinlock_lock(&inst->lock);
    
    inotify_watch_t** prev = &inst->watches;
    inotify_watch_t* watch = inst->watches;
    
    while (watch) {
        if (watch->wd == wd) {
            *prev = watch->next;
            kfree(watch);
            spinlock_unlock(&inst->lock);
            DEBUG_INFO("Inotify watch removed: wd=%d", wd);
            return 0;
        }
        prev = &watch->next;
        watch = watch->next;
    }
    
    spinlock_unlock(&inst->lock);
    return -1;
}

ssize_t inotify_read(i32 fd, void* buf, size_t count) {
    VALIDATE_PTR_RET(buf, -1);
    
    inotify_instance_t* inst = inotify_get_instance(fd);
    if (!inst) {
        return -1;
    }
    
    spinlock_lock(&inst->lock);
    
    if (inst->queue_count == 0) {
        spinlock_unlock(&inst->lock);
        return 0; /* No events */
    }
    
    /* Read first event from queue */
    inotify_event_t* event = inst->event_queue[inst->queue_head];
    if (!event) {
        spinlock_unlock(&inst->lock);
        return 0;
    }
    
    size_t event_size = sizeof(inotify_event_t) + event->len;
    if (count < event_size) {
        spinlock_unlock(&inst->lock);
        return -1; /* Buffer too small */
    }
    
    memcpy(buf, event, event_size);
    kfree(event);
    
    inst->queue_head = (inst->queue_head + 1) % INOTIFY_EVENT_QUEUE_SIZE;
    inst->queue_count--;
    
    spinlock_unlock(&inst->lock);
    return (ssize_t)event_size;
}

/* Internal function to queue event */
void inotify_queue_event(i32 wd, u32 mask, const char* name) {
    for (u32 i = 0; i < MAX_INOTIFY_INSTANCES; i++) {
        if (!inotify_instances[i].active) continue;
        
        inotify_instance_t* inst = &inotify_instances[i];
        spinlock_lock(&inst->lock);
        
        /* Check if any watch matches */
        inotify_watch_t* watch = inst->watches;
        while (watch) {
            if (watch->wd == wd && (watch->mask & mask)) {
                /* Queue event */
                if (inst->queue_count < INOTIFY_EVENT_QUEUE_SIZE) {
                    size_t name_len = name ? strlen(name) + 1 : 0;
                    inotify_event_t* event = (inotify_event_t*)kmalloc(sizeof(inotify_event_t) + name_len);
                    if (event) {
                        event->wd = wd;
                        event->mask = mask;
                        event->cookie = 0;
                        event->len = name_len;
                        if (name) {
                            memcpy(event->name, name, name_len);
                        }
                        
                        u32 tail = (inst->queue_tail + 1) % INOTIFY_EVENT_QUEUE_SIZE;
                        inst->event_queue[inst->queue_tail] = event;
                        inst->queue_tail = tail;
                        inst->queue_count++;
                    }
                }
                break;
            }
            watch = watch->next;
        }
        
        spinlock_unlock(&inst->lock);
    }
}
