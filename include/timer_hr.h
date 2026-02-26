#ifndef TIMER_HR_H
#define TIMER_HR_H

#include "types.h"

/* High-resolution timer */
typedef struct hrtimer {
    u64 expires;
    void (*function)(void*);
    void* data;
    struct hrtimer* next;
} hrtimer_t;

/* Initialize high-resolution timers */
void hrtimer_init(void);

/* Add timer */
hrtimer_t* hrtimer_add(u64 expires_ns, void (*function)(void*), void* data);

/* Remove timer */
void hrtimer_remove(hrtimer_t* timer);

/* Process timers */
void hrtimer_process(void);

/* Get current time (nanoseconds) */
u64 hrtimer_get_time(void);

#endif /* TIMER_HR_H */
