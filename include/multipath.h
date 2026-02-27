#ifndef MULTIPATH_H
#define MULTIPATH_H

#include "types.h"
#include "net.h"
#include "route.h"

/* Initialize multipath routing */
void multipath_init(void);

/* Add multipath route */
int multipath_add_route(ip_addr_t dst, ip_addr_t netmask, 
                        route_entry_t** paths, u32 num_paths);

/* Select path for destination */
route_entry_t* multipath_select_path(ip_addr_t dst);

/* Update path statistics */
void multipath_update_stats(ip_addr_t dst, u32 path_index, u64 bytes);

#endif /* MULTIPATH_H */
