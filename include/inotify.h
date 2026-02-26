#ifndef INOTIFY_H
#define INOTIFY_H

#include "types.h"

/* Inotify event masks */
#define IN_ACCESS         0x00000001
#define IN_MODIFY         0x00000002
#define IN_ATTRIB         0x00000004
#define IN_CLOSE_WRITE    0x00000008
#define IN_CLOSE_NOWRITE  0x00000010
#define IN_OPEN           0x00000020
#define IN_MOVED_FROM     0x00000040
#define IN_MOVED_TO       0x00000080
#define IN_CREATE         0x00000100
#define IN_DELETE         0x00000200
#define IN_DELETE_SELF    0x00000400
#define IN_MOVE_SELF      0x00000800
#define IN_UNMOUNT        0x00002000
#define IN_Q_OVERFLOW     0x00004000
#define IN_IGNORED        0x00008000
#define IN_ONLYDIR        0x01000000
#define IN_DONT_FOLLOW    0x02000000
#define IN_MASK_ADD       0x20000000
#define IN_ISDIR          0x40000000
#define IN_ONESHOT        0x80000000

/* Inotify event structure */
typedef struct inotify_event {
    i32 wd;         /* Watch descriptor */
    u32 mask;       /* Event mask */
    u32 cookie;     /* Cookie for rename events */
    u32 len;        /* Length of name */
    char name[];    /* Optional null-terminated name */
} inotify_event_t;

/* Inotify watch descriptor */
typedef struct inotify_watch {
    i32 wd;
    u32 mask;
    char path[256];
    struct inotify_watch* next;
} inotify_watch_t;

/* Initialize inotify */
int inotify_init(void);

/* Add watch */
i32 inotify_add_watch(i32 fd, const char* pathname, u32 mask);

/* Remove watch */
int inotify_rm_watch(i32 fd, i32 wd);

/* Read events */
ssize_t inotify_read(i32 fd, void* buf, size_t count);

#endif /* INOTIFY_H */
