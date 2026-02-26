#include "epoll.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_EPOLL_INSTANCES 128

static epoll_t epoll_instances[MAX_EPOLL_INSTANCES];
static u32 next_epfd = 1;

void epoll_init(void) {
    memset(epoll_instances, 0, sizeof(epoll_instances));
    DEBUG_INFO("Epoll system initialized");
}

int epoll_create(int size) {
    if (size <= 0) size = 1;
    
    for (u32 i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (epoll_instances[i].epfd == 0) {
            epoll_t* ep = &epoll_instances[i];
            ep->epfd = next_epfd++;
            ep->max_events = size;
            ep->events = (epoll_event_t*)kzalloc(size * sizeof(epoll_event_t));
            ep->event_count = 0;
            spinlock_init(&ep->lock);
            return ep->epfd;
        }
    }
    
    return -1;
}

int epoll_ctl(int epfd, int op, int fd, epoll_event_t* event) {
    (void)op;
    (void)fd;
    (void)event;
    
    for (u32 i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (epoll_instances[i].epfd == epfd) {
            /* Would add/modify/remove file descriptor */
            return 0;
        }
    }
    
    return -1;
}

int epoll_wait(int epfd, epoll_event_t* events, int maxevents, int timeout) {
    (void)timeout;
    
    for (u32 i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (epoll_instances[i].epfd == epfd) {
            epoll_t* ep = &epoll_instances[i];
            spinlock_lock(&ep->lock);
            
            int count = (ep->event_count < maxevents) ? ep->event_count : maxevents;
            if (count > 0) {
                memcpy(events, ep->events, count * sizeof(epoll_event_t));
                ep->event_count = 0;
            }
            
            spinlock_unlock(&ep->lock);
            return count;
        }
    }
    
    return -1;
}

int select(int nfds, fd_set_t* readfds, fd_set_t* writefds, fd_set_t* exceptfds, void* timeout) {
    (void)nfds;
    (void)readfds;
    (void)writefds;
    (void)exceptfds;
    (void)timeout;
    /* Would wait for file descriptors */
    return 0;
}
