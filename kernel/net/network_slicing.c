#include "network_slicing.h"
#include "net.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_NETWORK_SLICES 32
#define MAX_SLICE_RULES 256

/* Network slice */
typedef struct network_slice {
    u32 slice_id;
    char name[64];
    u64 bandwidth_limit;    /* Bits per second */
    u64 latency_target;     /* Target latency in microseconds */
    u64 priority;           /* Slice priority */
    u64 packets;
    u64 bytes;
    bool active;
    struct network_slice* next;
} network_slice_t;

/* Slice rule */
typedef struct slice_rule {
    u32 slice_id;
    ip_addr_t src;
    ip_addr_t dst;
    ip_addr_t src_mask;
    ip_addr_t dst_mask;
    u8 protocol;
    u16 src_port;
    u16 dst_port;
    struct slice_rule* next;
} slice_rule_t;

static network_slice_t* slices = NULL;
static slice_rule_t* slice_rules = NULL;
static spinlock_t slicing_lock = SPINLOCK_INIT;
static u32 slice_counter = 0;

void network_slicing_init(void) {
    slices = NULL;
    slice_rules = NULL;
    slice_counter = 0;
    printk("[Network Slicing] Network slicing initialized\n");
}

u32 network_slice_create(const char* name, u64 bandwidth_limit, 
                         u64 latency_target, u64 priority) {
    if (!name || slice_counter >= MAX_NETWORK_SLICES) {
        return 0;
    }
    
    network_slice_t* slice = (network_slice_t*)kzalloc(sizeof(network_slice_t));
    if (!slice) {
        return 0;
    }
    
    slice->slice_id = ++slice_counter;
    strncpy(slice->name, name, sizeof(slice->name) - 1);
    slice->bandwidth_limit = bandwidth_limit;
    slice->latency_target = latency_target;
    slice->priority = priority;
    slice->packets = 0;
    slice->bytes = 0;
    slice->active = true;
    
    spinlock_lock(&slicing_lock);
    slice->next = slices;
    slices = slice;
    spinlock_unlock(&slicing_lock);
    
    DEBUG_INFO("Network slice created: %s (id=%u)", name, slice->slice_id);
    return slice->slice_id;
}

void network_slice_destroy(u32 slice_id) {
    spinlock_lock(&slicing_lock);
    
    network_slice_t* prev = NULL;
    network_slice_t* slice = slices;
    while (slice) {
        if (slice->slice_id == slice_id) {
            if (prev) {
                prev->next = slice->next;
            } else {
                slices = slice->next;
            }
            slice->active = false;
            kfree(slice);
            break;
        }
        prev = slice;
        slice = slice->next;
    }
    
    /* Remove all rules for this slice */
    slice_rule_t* prev_rule = NULL;
    slice_rule_t* rule = slice_rules;
    while (rule) {
        if (rule->slice_id == slice_id) {
            if (prev_rule) {
                prev_rule->next = rule->next;
            } else {
                slice_rules = rule->next;
            }
            slice_rule_t* to_free = rule;
            rule = rule->next;
            kfree(to_free);
        } else {
            prev_rule = rule;
            rule = rule->next;
        }
    }
    
    spinlock_unlock(&slicing_lock);
}

int network_slice_add_rule(u32 slice_id, ip_addr_t src, ip_addr_t src_mask,
                            ip_addr_t dst, ip_addr_t dst_mask,
                            u8 protocol, u16 src_port, u16 dst_port) {
    if (slice_id == 0) {
        return -1;
    }
    
    slice_rule_t* rule = (slice_rule_t*)kzalloc(sizeof(slice_rule_t));
    if (!rule) {
        return -1;
    }
    
    rule->slice_id = slice_id;
    rule->src = src;
    rule->src_mask = src_mask;
    rule->dst = dst;
    rule->dst_mask = dst_mask;
    rule->protocol = protocol;
    rule->src_port = src_port;
    rule->dst_port = dst_port;
    
    spinlock_lock(&slicing_lock);
    rule->next = slice_rules;
    slice_rules = rule;
    spinlock_unlock(&slicing_lock);
    
    DEBUG_INFO("Slice rule added: slice_id=%u", slice_id);
    return 0;
}

u32 network_slice_classify(sk_buff_t* skb) {
    if (!skb) {
        return 0;
    }
    
    /* Extract packet information */
    ip_header_t* iph = (ip_header_t*)skb->data;
    if (skb->len < sizeof(ip_header_t)) {
        return 0;
    }
    
    spinlock_lock(&slicing_lock);
    
    slice_rule_t* rule = slice_rules;
    while (rule) {
        /* Check if packet matches rule */
        bool matches = true;
        
        /* Check source IP */
        for (int i = 0; i < 4; i++) {
            if ((iph->src.addr[i] & rule->src_mask.addr[i]) != 
                (rule->src.addr[i] & rule->src_mask.addr[i])) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            /* Check destination IP */
            for (int i = 0; i < 4; i++) {
                if ((iph->dst.addr[i] & rule->dst_mask.addr[i]) != 
                    (rule->dst.addr[i] & rule->dst_mask.addr[i])) {
                    matches = false;
                    break;
                }
            }
        }
        
        if (matches && rule->protocol != 0 && iph->protocol != rule->protocol) {
            matches = false;
        }
        
        if (matches) {
            spinlock_unlock(&slicing_lock);
            return rule->slice_id;
        }
        
        rule = rule->next;
    }
    
    spinlock_unlock(&slicing_lock);
    return 0; /* Default slice */
}

network_slice_t* network_slice_get(u32 slice_id) {
    spinlock_lock(&slicing_lock);
    
    network_slice_t* slice = slices;
    while (slice) {
        if (slice->slice_id == slice_id && slice->active) {
            spinlock_unlock(&slicing_lock);
            return slice;
        }
        slice = slice->next;
    }
    
    spinlock_unlock(&slicing_lock);
    return NULL;
}

void network_slice_update_stats(u32 slice_id, u64 bytes) {
    network_slice_t* slice = network_slice_get(slice_id);
    if (slice) {
        slice->packets++;
        slice->bytes += bytes;
    }
}

bool network_slice_check_bandwidth(u32 slice_id, u64 bytes) {
    network_slice_t* slice = network_slice_get(slice_id);
    if (!slice) {
        return true;
    }
    
    /* Simplified bandwidth check */
    u64 current_rate = (slice->bytes * 8) / 1000; /* Bits per millisecond */
    u64 limit_rate = slice->bandwidth_limit / 1000;
    
    return current_rate < limit_rate;
}
