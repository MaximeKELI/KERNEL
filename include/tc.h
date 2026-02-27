#ifndef TC_H
#define TC_H

#include "types.h"
#include "skbuff.h"

/* Initialize Traffic Control */
void tc_init(void);

/* Add/delete qdisc */
int tc_add_qdisc(const char* name, u32 handle, u32 parent);
int tc_del_qdisc(u32 handle);

/* Add class to qdisc */
int tc_add_class(u32 qdisc_handle, u32 classid, u64 rate, u64 ceil, u64 burst);

/* Enqueue packet (rate limiting) */
int tc_enqueue(sk_buff_t* skb, u32 qdisc_handle, u32 classid);

#endif /* TC_H */
