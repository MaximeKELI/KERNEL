#ifndef NETFILTER_H
#define NETFILTER_H

#include "types.h"
#include "net.h"
#include "skbuff.h"

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

/* Initialize Netfilter */
void netfilter_init(void);

/* Register hook function */
typedef u32 (*nf_hookfn_t)(u32 hook, sk_buff_t* skb, netif_t* in, 
                           netif_t* out, int (*okfn)(sk_buff_t*));
int netfilter_register_hook(u32 hook, nf_hookfn_t fn);

/* Call netfilter hook */
u32 netfilter_hook(u32 hook, sk_buff_t* skb, netif_t* in, netif_t* out);

/* Add/delete rule */
int netfilter_add_rule(ip_addr_t src, ip_addr_t src_mask,
                       ip_addr_t dst, ip_addr_t dst_mask,
                       u8 protocol, u16 src_port, u16 dst_port,
                       u32 hook, u32 target);
int netfilter_del_rule(u32 hook, u32 target);

#endif /* NETFILTER_H */
