#ifndef TIMER_H
#define TIMER_H

#include "types.h"

/* Timer callback */
typedef void (*timer_callback_t)(void* data);

/* Initialize timer */
void timer_init(u32 frequency);

/* Get tick count */
u64 timer_get_ticks(void);

/* Sleep (milliseconds) */
void timer_sleep(u64 ms);

/* Register timer callback */
void timer_register_callback(timer_callback_t callback, void* data);

#endif /* TIMER_H */
