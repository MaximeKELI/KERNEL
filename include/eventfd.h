#ifndef EVENTFD_H
#define EVENTFD_H

#include "types.h"

/* Eventfd flags */
#define EFD_CLOEXEC    0x00000001
#define EFD_NONBLOCK   0x00000002
#define EFD_SEMAPHORE  0x00000004

/* Create eventfd */
i32 eventfd(u32 initval, i32 flags);

/* Read from eventfd */
ssize_t eventfd_read(i32 fd, u64* value);

/* Write to eventfd */
ssize_t eventfd_write(i32 fd, u64 value);

#endif /* EVENTFD_H */
