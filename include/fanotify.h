#ifndef FANOTIFY_H
#define FANOTIFY_H

#include "types.h"

/* Fanotify event masks */
#define FAN_ACCESS         0x00000001
#define FAN_MODIFY         0x00000002
#define FAN_CLOSE_WRITE    0x00000008
#define FAN_CLOSE_NOWRITE  0x00000010
#define FAN_OPEN           0x00000020
#define FAN_OPEN_EXEC      0x00001000
#define FAN_ATTRIB         0x00000004
#define FAN_MOVED_FROM     0x00000040
#define FAN_MOVED_TO       0x00000080
#define FAN_CREATE         0x00000100
#define FAN_DELETE         0x00000200
#define FAN_DELETE_SELF    0x00000400
#define FAN_MOVE_SELF      0x00000800
#define FAN_UNMOUNT        0x00002000
#define FAN_Q_OVERFLOW     0x00004000
#define FAN_ONDIR          0x40000000
#define FAN_EVENT_ON_CHILD 0x08000000

/* Fanotify flags */
#define FAN_CLOEXEC         0x00000001
#define FAN_NONBLOCK        0x00000002
#define FAN_CLASS_NOTIF     0x00000000
#define FAN_CLASS_CONTENT   0x00000004
#define FAN_CLASS_PRE_CONTENT 0x00000008
#define FAN_UNLIMITED_QUEUE 0x00000010
#define FAN_UNLIMITED_MARKS 0x00000020

/* Fanotify mark flags */
#define FAN_MARK_ADD        0x00000001
#define FAN_MARK_REMOVE     0x00000002
#define FAN_MARK_DONT_FOLLOW 0x00000004
#define FAN_MARK_ONLYDIR    0x00000008
#define FAN_MARK_MOUNT      0x00000010
#define FAN_MARK_IGNORED_MASK 0x00000020
#define FAN_MARK_IGNORED_SURV_MODIFY 0x00000040
#define FAN_MARK_FLUSH      0x00000080

/* Fanotify event structure */
typedef struct fanotify_event_metadata {
    u32 event_len;
    u8 vers;
    u8 reserved;
    u16 metadata_len;
    u64 mask;
    i32 fd;
    i32 pid;
} fanotify_event_metadata_t;

/* Initialize fanotify */
int fanotify_init(u32 flags, u32 event_f_flags);

/* Add mark */
int fanotify_mark(i32 fanotify_fd, u32 flags, u64 mask, i32 dirfd, const char* pathname);

/* Read events */
ssize_t fanotify_read(i32 fd, void* buf, size_t count);

#endif /* FANOTIFY_H */
