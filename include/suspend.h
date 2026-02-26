#ifndef SUSPEND_H
#define SUSPEND_H

#include "types.h"

/* Suspend states */
#define SUSPEND_STATE_FREEZE   0
#define SUSPEND_STATE_STANDBY  1
#define SUSPEND_STATE_MEM      2
#define SUSPEND_STATE_DISK     3

/* Suspend flags */
#define SUSPEND_FLAG_NONE      0
#define SUSPEND_FLAG_TEST      0x01
#define SUSPEND_FLAG_NO_PLATFORM 0x02

/* Initialize suspend system */
void suspend_init(void);

/* Suspend to RAM */
int suspend_to_ram(void);

/* Suspend to disk (hibernation) */
int suspend_to_disk(void);

/* Resume from suspend */
int resume_from_suspend(void);

/* Resume from hibernation */
int resume_from_hibernation(void);

/* Enter suspend state */
int suspend_enter(u32 state, u32 flags);

#endif /* SUSPEND_H */
