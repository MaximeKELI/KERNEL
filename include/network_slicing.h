#ifndef NETWORK_SLICING_H
#define NETWORK_SLICING_H

#include "types.h"
#include "skbuff.h"
#include "net.h"

void network_slicing_init(void);
u32 network_slice_create(const char* name, u64 bandwidth_limit,
                         u64 latency_target, u64 priority);
void network_slice_destroy(u32 slice_id);
int network_slice_add_rule(u32 slice_id, ip_addr_t src, ip_addr_t src_mask,
                           ip_addr_t dst, ip_addr_t dst_mask, u16 src_port,
                           u16 dst_port, u8 protocol);
u32 network_slice_classify(sk_buff_t* skb);
void network_slice_update_stats(u32 slice_id, u64 bytes);
bool network_slice_check_bandwidth(u32 slice_id, u64 bytes);

#endif /* NETWORK_SLICING_H */
