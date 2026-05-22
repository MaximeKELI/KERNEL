#ifndef TIMERFD_H
#define TIMERFD_H

#include "types.h"

/* Timerfd flags */
#define TFD_CLOEXEC     0x00000001
#define TFD_NONBLOCK    0x00000002

/* Timerfd clock types */
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

/* Timerfd timer types */
#define TFD_TIMER_ABSTIME 0x00000001

typedef struct timespec {
    i64 tv_sec;   /* Seconds */
    i64 tv_nsec;  /* Nanoseconds */
} timespec_t;

/* Timerfd structure */
typedef struct itimerspec {
    timespec_t it_interval;
    timespec_t it_value;
} itimerspec_t;

/* Create timerfd */
i32 timerfd_create(i32 clockid, i32 flags);

/* Set timer */
int timerfd_settime(i32 fd, i32 flags, const itimerspec_t* new_value, itimerspec_t* old_value);

/* Get timer */
int timerfd_gettime(i32 fd, itimerspec_t* curr_value);

/* Read from timerfd */
ssize_t timerfd_read(i32 fd, u64* expirations);

#endif /* TIMERFD_H */
