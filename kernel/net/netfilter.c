#include "netfilter.h"
#include "net.h"
#include "skbuff.h"
#include "ip.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_NETFILTER_RULES 1024

/* Netfilter hooks */
#define NF_INET_PRE_ROUTING  0
#define NF_INET_LOCAL_IN     1
#define NF_INET_FORWARD      2
#define NF_INET_LOCAL_OUT    3
#define NF_INET_POST_ROUTING 4

/* Netfilter verdicts */
#define NF_ACCEPT 1
#define NF_DROP   0
#define NF_STOLEN 2
#define NF_QUEUE  3
#define NF_REPEAT 4

/* Netfilter rule */
typedef struct nf_rule {
    ip_addr_t src;
    ip_addr_t dst;
    ip_addr_t src_mask;
    ip_addr_t dst_mask;
    u8 protocol;
    u16 src_port;
    u16 dst_port;
    u32 hook;
    u32 target;  /* NF_ACCEPT, NF_DROP, etc. */
    u32 priority;
    struct nf_rule* next;
} nf_rule_t;

/* Netfilter hook function */
typedef u32 (*nf_hookfn_t)(u32 hook, sk_buff_t* skb, netif_t* in, 
                           netif_t* out, int (*okfn)(sk_buff_t*));

static nf_rule_t* nf_rules = NULL;
static nf_hookfn_t nf_hooks[5][16];
static u32 nf_hook_count[5] = {0};
static spinlock_t nf_lock = SPINLOCK_INIT;

void netfilter_init(void) {
    memset(nf_hooks, 0, sizeof(nf_hooks));
    memset(nf_hook_count, 0, sizeof(nf_hook_count));
    nf_rules = NULL;
    
    printk("[Netfilter] Netfilter framework initialized\n");
}

int netfilter_register_hook(u32 hook, nf_hookfn_t fn) {
    if (hook >= 5 || !fn) {
        return -1;
    }
    
    spinlock_lock(&nf_lock);
    
    if (nf_hook_count[hook] >= 16) {
        spinlock_unlock(&nf_lock);
        return -1;
    }
    
    nf_hooks[hook][nf_hook_count[hook]++] = fn;
    
    spinlock_unlock(&nf_lock);
    
    DEBUG_INFO("Netfilter hook registered: hook=%u", hook);
    return 0;
}

u32 netfilter_hook(u32 hook, sk_buff_t* skb, netif_t* in, netif_t* out) {
    if (hook >= 5 || !skb) {
        return NF_DROP;
    }
    
    spinlock_lock(&nf_lock);
    
    /* Call all hooks */
    for (u32 i = 0; i < nf_hook_count[hook]; i++) {
        if (nf_hooks[hook][i]) {
            u32 verdict = nf_hooks[hook][i](hook, skb, in, out, NULL);
            if (verdict != NF_ACCEPT) {
                spinlock_unlock(&nf_lock);
                return verdict;
            }
        }
    }
    
    /* Check rules */
    nf_rule_t* rule = nf_rules;
    while (rule) {
        if (rule->hook == hook) {
            /* Check if packet matches rule */
            bool matches = true;
            
            if (rule->protocol != 0) {
                ip_header_t* iph = (ip_header_t*)skb->data;
                if (iph->protocol != rule->protocol) {
                    matches = false;
                }
            }
            
            if (matches) {
                spinlock_unlock(&nf_lock);
                return rule->target;
            }
        }
        rule = rule->next;
    }
    
    spinlock_unlock(&nf_lock);
    return NF_ACCEPT;
}

int netfilter_add_rule(ip_addr_t src, ip_addr_t src_mask,
                       ip_addr_t dst, ip_addr_t dst_mask,
                       u8 protocol, u16 src_port, u16 dst_port,
                       u32 hook, u32 target) {
    nf_rule_t* rule = (nf_rule_t*)kzalloc(sizeof(nf_rule_t));
    if (!rule) {
        return -1;
    }
    
    rule->src = src;
    rule->src_mask = src_mask;
    rule->dst = dst;
    rule->dst_mask = dst_mask;
    rule->protocol = protocol;
    rule->src_port = src_port;
    rule->dst_port = dst_port;
    rule->hook = hook;
    rule->target = target;
    rule->priority = 0;
    
    spinlock_lock(&nf_lock);
    rule->next = nf_rules;
    nf_rules = rule;
    spinlock_unlock(&nf_lock);
    
    DEBUG_INFO("Netfilter rule added: hook=%u, target=%u", hook, target);
    return 0;
}

int netfilter_del_rule(u32 hook, u32 target) {
    spinlock_lock(&nf_lock);
    
    nf_rule_t* prev = NULL;
    nf_rule_t* rule = nf_rules;
    
    while (rule) {
        if (rule->hook == hook && rule->target == target) {
            if (prev) {
                prev->next = rule->next;
            } else {
                nf_rules = rule->next;
            }
            kfree(rule);
            spinlock_unlock(&nf_lock);
            return 0;
        }
        prev = rule;
        rule = rule->next;
    }
    
    spinlock_unlock(&nf_lock);
    return -1;
}
