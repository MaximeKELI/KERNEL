#ifndef DNOTIFY_H
#define DNOTIFY_H

#include "types.h"

/* Dnotify flags */
#define DN_ACCESS         0x00000001
#define DN_MODIFY           0x00000002
#define DN_CREATE           0x00000004
#define DN_DELETE           0x00000008
#define DN_RENAME           0x00000010
#define DN_ATTRIB           0x00000020
#define DN_MULTISHOT        0x80000000

/* Initialize dnotify */
int dnotify_init(i32 fd, u32 mask);

/* Get events */
u32 dnotify_get_events(i32 fd);

/* Clear events */
void dnotify_clear_events(i32 fd, u32 mask);

#endif /* DNOTIFY_H */
