#include "multipath.h"
#include "net.h"
#include "route.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_MULTIPATH_ROUTES 64

/* Multipath route */
typedef struct multipath_route {
    ip_addr_t dst;
    ip_addr_t netmask;
    u32 num_paths;
    route_entry_t* paths[MAX_MULTIPATH_ROUTES];
    u32 current_path;
    u64 path_bytes[MAX_MULTIPATH_ROUTES];
    u64 path_packets[MAX_MULTIPATH_ROUTES];
    struct multipath_route* next;
} multipath_route_t;

static multipath_route_t* multipath_routes = NULL;
static spinlock_t multipath_lock = SPINLOCK_INIT;

void multipath_init(void) {
    multipath_routes = NULL;
    printk("[Multipath] Multipath routing initialized\n");
}

int multipath_add_route(ip_addr_t dst, ip_addr_t netmask, route_entry_t** paths, u32 num_paths) {
    if (!paths || num_paths == 0 || num_paths > MAX_MULTIPATH_ROUTES) {
        return -1;
    }
    
    multipath_route_t* route = (multipath_route_t*)kzalloc(sizeof(multipath_route_t));
    if (!route) {
        return -1;
    }
    
    route->dst = dst;
    route->netmask = netmask;
    route->num_paths = num_paths;
    memcpy(route->paths, paths, num_paths * sizeof(route_entry_t*));
    route->current_path = 0;
    memset(route->path_bytes, 0, sizeof(route->path_bytes));
    memset(route->path_packets, 0, sizeof(route->path_packets));
    
    spinlock_lock(&multipath_lock);
    route->next = multipath_routes;
    multipath_routes = route;
    spinlock_unlock(&multipath_lock);
    
    DEBUG_INFO("Multipath route added: %u paths", num_paths);
    return 0;
}

route_entry_t* multipath_select_path(ip_addr_t dst) {
    spinlock_lock(&multipath_lock);
    
    multipath_route_t* route = multipath_routes;
    while (route) {
        /* Check if destination matches */
        bool matches = true;
        for (int i = 0; i < 4; i++) {
            if ((dst.addr[i] & route->netmask.addr[i]) != 
                (route->dst.addr[i] & route->netmask.addr[i])) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            /* Select path using round-robin */
            u32 path = route->current_path;
            route->current_path = (route->current_path + 1) % route->num_paths;
            
            spinlock_unlock(&multipath_lock);
            return route->paths[path];
        }
        
        route = route->next;
    }
    
    spinlock_unlock(&multipath_lock);
    return NULL;
}

void multipath_update_stats(ip_addr_t dst, u32 path_index, u64 bytes) {
    spinlock_lock(&multipath_lock);
    
    multipath_route_t* route = multipath_routes;
    while (route) {
        bool matches = true;
        for (int i = 0; i < 4; i++) {
            if ((dst.addr[i] & route->netmask.addr[i]) != 
                (route->dst.addr[i] & route->netmask.addr[i])) {
                matches = false;
                break;
            }
        }
        
        if (matches && path_index < route->num_paths) {
            route->path_bytes[path_index] += bytes;
            route->path_packets[path_index]++;
            break;
        }
        
        route = route->next;
    }
    
    spinlock_unlock(&multipath_lock);
}
