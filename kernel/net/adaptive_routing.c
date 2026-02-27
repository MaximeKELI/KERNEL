#include "adaptive_routing.h"
#include "route.h"
#include "net.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_ADAPTIVE_ROUTES 256
#define LATENCY_HISTORY_SIZE 64

/* Latency measurement */
typedef struct {
    u64 latencies[LATENCY_HISTORY_SIZE];
    u32 index;
    u64 avg_latency;
    u64 min_latency;
    u64 max_latency;
    u64 packet_count;
} latency_stats_t;

/* Adaptive route */
typedef struct adaptive_route {
    ip_addr_t dst;
    ip_addr_t netmask;
    route_entry_t* paths[MAX_ADAPTIVE_ROUTES];
    u32 num_paths;
    latency_stats_t path_latency[MAX_ADAPTIVE_ROUTES];
    u32 best_path;
    u64 last_update;
    struct adaptive_route* next;
} adaptive_route_t;

static adaptive_route_t* adaptive_routes = NULL;
static spinlock_t adaptive_lock = SPINLOCK_INIT;
static bool adaptive_enabled = true;

void adaptive_routing_init(void) {
    adaptive_routes = NULL;
    adaptive_enabled = true;
    printk("[Adaptive Routing] Adaptive routing with latency-based selection initialized\n");
}

static void update_latency_stats(latency_stats_t* stats, u64 latency) {
    stats->latencies[stats->index] = latency;
    stats->index = (stats->index + 1) % LATENCY_HISTORY_SIZE;
    stats->packet_count++;
    
    /* Calculate average */
    u64 sum = 0;
    u32 count = stats->packet_count < LATENCY_HISTORY_SIZE ? 
                 stats->packet_count : LATENCY_HISTORY_SIZE;
    
    for (u32 i = 0; i < count; i++) {
        sum += stats->latencies[i];
    }
    
    stats->avg_latency = sum / count;
    
    /* Update min/max */
    if (latency < stats->min_latency || stats->min_latency == 0) {
        stats->min_latency = latency;
    }
    if (latency > stats->max_latency) {
        stats->max_latency = latency;
    }
}

int adaptive_routing_add_route(ip_addr_t dst, ip_addr_t netmask, 
                                 route_entry_t** paths, u32 num_paths) {
    if (!paths || num_paths == 0 || num_paths > MAX_ADAPTIVE_ROUTES) {
        return -1;
    }
    
    adaptive_route_t* route = (adaptive_route_t*)kzalloc(sizeof(adaptive_route_t));
    if (!route) {
        return -1;
    }
    
    route->dst = dst;
    route->netmask = netmask;
    route->num_paths = num_paths;
    memcpy(route->paths, paths, num_paths * sizeof(route_entry_t*));
    route->best_path = 0;
    route->last_update = 0; /* TODO: Use actual timestamp */
    
    spinlock_lock(&adaptive_lock);
    route->next = adaptive_routes;
    adaptive_routes = route;
    spinlock_unlock(&adaptive_lock);
    
    DEBUG_INFO("Adaptive route added: %u paths", num_paths);
    return 0;
}

route_entry_t* adaptive_routing_select_path(ip_addr_t dst) {
    if (!adaptive_enabled) {
        return route_lookup(dst);
    }
    
    spinlock_lock(&adaptive_lock);
    
    adaptive_route_t* route = adaptive_routes;
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
            /* Select path with lowest average latency */
            u32 best = 0;
            u64 best_latency = route->path_latency[0].avg_latency;
            
            for (u32 i = 1; i < route->num_paths; i++) {
                if (route->path_latency[i].avg_latency < best_latency ||
                    best_latency == 0) {
                    best = i;
                    best_latency = route->path_latency[i].avg_latency;
                }
            }
            
            route->best_path = best;
            route_entry_t* selected = route->paths[best];
            
            spinlock_unlock(&adaptive_lock);
            return selected;
        }
        
        route = route->next;
    }
    
    spinlock_unlock(&adaptive_lock);
    return route_lookup(dst);
}

void adaptive_routing_update_latency(ip_addr_t dst, u32 path_index, u64 latency) {
    spinlock_lock(&adaptive_lock);
    
    adaptive_route_t* route = adaptive_routes;
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
            update_latency_stats(&route->path_latency[path_index], latency);
            break;
        }
        
        route = route->next;
    }
    
    spinlock_unlock(&adaptive_lock);
}

void adaptive_routing_get_stats(ip_addr_t dst, u32 path_index,
                                 u64* avg_latency, u64* min_latency, 
                                 u64* max_latency, u64* packet_count) {
    spinlock_lock(&adaptive_lock);
    
    adaptive_route_t* route = adaptive_routes;
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
            latency_stats_t* stats = &route->path_latency[path_index];
            if (avg_latency) *avg_latency = stats->avg_latency;
            if (min_latency) *min_latency = stats->min_latency;
            if (max_latency) *max_latency = stats->max_latency;
            if (packet_count) *packet_count = stats->packet_count;
            break;
        }
        
        route = route->next;
    }
    
    spinlock_unlock(&adaptive_lock);
}

void adaptive_routing_enable(void) {
    adaptive_enabled = true;
    printk("[Adaptive Routing] Enabled\n");
}

void adaptive_routing_disable(void) {
    adaptive_enabled = false;
    printk("[Adaptive Routing] Disabled\n");
}

bool adaptive_routing_is_enabled(void) {
    return adaptive_enabled;
}
