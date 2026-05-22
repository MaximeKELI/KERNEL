#ifndef EPOLL_H
#define EPOLL_H

#include "types.h"
#include "spinlock.h"

/* Epoll events */
#define EPOLLIN  0x001
#define EPOLLOUT 0x004
#define EPOLLERR 0x008
#define EPOLLHUP 0x010
#define EPOLLET  0x80000000

/* Epoll event */
typedef struct epoll_event {
    u32 events;
    u64 data;
} epoll_event_t;

/* Epoll instance */
typedef struct epoll {
    u32 epfd;
    epoll_event_t* events;
    u32 max_events;
    u32 event_count;
    spinlock_t lock;
} epoll_t;

/* Initialize epoll */
void epoll_init(void);

/* Create epoll instance */
int epoll_create(int size);

/* Control epoll */
int epoll_ctl(int epfd, int op, int fd, epoll_event_t* event);

/* Wait for events */
int epoll_wait(int epfd, epoll_event_t* events, int maxevents, int timeout);

/* Select (alternative) */
typedef struct {
    u64* readfds;
    u64* writefds;
    u64* exceptfds;
} fd_set_t;

int select(int nfds, fd_set_t* readfds, fd_set_t* writefds, fd_set_t* exceptfds, void* timeout);

#endif /* EPOLL_H */
