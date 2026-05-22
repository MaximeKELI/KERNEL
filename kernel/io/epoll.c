#include "epoll.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "spinlock.h"
#include "tcp.h"
#include "net.h"

#define MAX_EPOLL_INSTANCES 64
#define MAX_EPOLL_FDS 128
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

typedef struct epoll_fd_entry {
    int fd;
    u32 events;
    bool used;
} epoll_fd_entry_t;

typedef struct epoll_instance {
    u32 epfd;
    epoll_fd_entry_t fds[MAX_EPOLL_FDS];
    u32 nfds;
    spinlock_t lock;
    bool used;
} epoll_instance_t;

static epoll_instance_t epoll_instances[MAX_EPOLL_INSTANCES];
static u32 next_epfd = 100;

extern int socket_fd_poll_events(int fd);

void epoll_init(void) {
    memset(epoll_instances, 0, sizeof(epoll_instances));
    next_epfd = 100;
}

int epoll_create(int size) {
    (void)size;
    for (u32 i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (!epoll_instances[i].used) {
            epoll_instance_t* ep = &epoll_instances[i];
            memset(ep, 0, sizeof(*ep));
            ep->epfd = next_epfd++;
            ep->used = true;
            spinlock_init(&ep->lock);
            return (int)ep->epfd;
        }
    }
    return -1;
}

static epoll_instance_t* epoll_find(int epfd) {
    for (u32 i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (epoll_instances[i].used && epoll_instances[i].epfd == (u32)epfd) {
            return &epoll_instances[i];
        }
    }
    return NULL;
}

int epoll_ctl(int epfd, int op, int fd, epoll_event_t* event) {
    epoll_instance_t* ep = epoll_find(epfd);
    if (!ep || !event) {
        return -1;
    }

    spinlock_lock(&ep->lock);
    if (op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD) {
        for (u32 i = 0; i < MAX_EPOLL_FDS; i++) {
            if (ep->fds[i].used && ep->fds[i].fd == fd) {
                ep->fds[i].events = event->events;
                spinlock_unlock(&ep->lock);
                return 0;
            }
        }
        for (u32 i = 0; i < MAX_EPOLL_FDS; i++) {
            if (!ep->fds[i].used) {
                ep->fds[i].used = true;
                ep->fds[i].fd = fd;
                ep->fds[i].events = event->events;
                ep->nfds++;
                spinlock_unlock(&ep->lock);
                return 0;
            }
        }
    } else if (op == EPOLL_CTL_DEL) {
        for (u32 i = 0; i < MAX_EPOLL_FDS; i++) {
            if (ep->fds[i].used && ep->fds[i].fd == fd) {
                ep->fds[i].used = false;
                if (ep->nfds > 0) {
                    ep->nfds--;
                }
                spinlock_unlock(&ep->lock);
                return 0;
            }
        }
    }
    spinlock_unlock(&ep->lock);
    return -1;
}

int epoll_wait(int epfd, epoll_event_t* events, int maxevents, int timeout) {
    epoll_instance_t* ep = epoll_find(epfd);
    if (!ep || !events || maxevents <= 0) {
        return -1;
    }

    u32 spins = timeout <= 0 ? 1 : (u32)timeout;
    int ready = 0;

    for (u32 round = 0; round < spins && ready < maxevents; round++) {
        spinlock_lock(&ep->lock);
        for (u32 i = 0; i < MAX_EPOLL_FDS && ready < maxevents; i++) {
            if (!ep->fds[i].used) {
                continue;
            }
            int revents = socket_fd_poll_events(ep->fds[i].fd);
            if (revents & ep->fds[i].events) {
                events[ready].events = revents & ep->fds[i].events;
                events[ready].data = (u64)ep->fds[i].fd;
                ready++;
            }
        }
        spinlock_unlock(&ep->lock);
        if (ready > 0) {
            break;
        }
        if (timeout != 0) {
            net_poll();
        }
    }
    return ready;
}

int select(int nfds, fd_set_t* readfds, fd_set_t* writefds, fd_set_t* exceptfds, void* timeout) {
    (void)nfds;
    (void)readfds;
    (void)writefds;
    (void)exceptfds;
    (void)timeout;
    return 0;
}
