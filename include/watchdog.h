#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "types.h"

/* Watchdog device */
typedef struct watchdog {
    char name[32];
    u32 timeout;
    bool running;
    void (*kick)(void);
    struct watchdog* next;
} watchdog_t;

/* Initialize watchdog */
void watchdog_init(void);

/* Register watchdog */
int watchdog_register(watchdog_t* wd);

/* Start watchdog */
int watchdog_start(u32 timeout);

/* Stop watchdog */
int watchdog_stop(void);

/* Kick watchdog */
void watchdog_kick(void);

#endif /* WATCHDOG_H */
