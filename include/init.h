#ifndef INIT_H
#define INIT_H

#include "types.h"

/* True once init (PID 1) owns the console; kshell must stand down. */
extern bool g_init_active;

/* Spawn init (PID 1): runs and respawns the ring-3 shell /sh. */
void init_start(void);

#endif /* INIT_H */
