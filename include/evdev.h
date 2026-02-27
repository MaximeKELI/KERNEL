#ifndef EVDEV_H
#define EVDEV_H

#include "types.h"

/* Input Event */
typedef struct input_event input_event_t;

/* Evdev Device */
typedef struct evdev_device evdev_device_t;

/* Initialize Evdev */
void evdev_init(void);

/* Allocate/free evdev device */
evdev_device_t* evdev_alloc_device(void);
void evdev_free_device(evdev_device_t* dev);

/* Write/read input events */
int evdev_write_event(evdev_device_t* dev, input_event_t* event);
int evdev_read_event(evdev_device_t* dev, input_event_t* event);

/* Find device by name */
evdev_device_t* evdev_find_device(const char* name);

/* Get device count */
u32 evdev_get_device_count(void);

#endif /* EVDEV_H */
