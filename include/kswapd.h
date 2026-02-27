#ifndef KSWAPD_H
#define KSWAPD_H

#include "types.h"

/* Initialize kswapd (kernel swap daemon) */
void kswapd_init(void);

/* Stop kswapd */
void kswapd_stop(void);

/* Get number of pages reclaimed */
u64 kswapd_get_reclaimed_pages(void);

/* Get number of reclaim cycles */
u64 kswapd_get_reclaim_cycles(void);

/* Check if kswapd is running */
bool kswapd_is_running(void);

/* Reclaim pages (called by kswapd) */
u64 memory_reclaim_pages(u64 count);

#endif /* KSWAPD_H */
