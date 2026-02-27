#ifndef PACKET_SCHED_H
#define PACKET_SCHED_H

#include "types.h"
#include "skbuff.h"

/* Packet scheduler queue */
typedef struct sched_queue sched_queue_t;

/* Initialize packet scheduler */
void packet_sched_init(void);

/* Create/destroy queue */
sched_queue_t* packet_sched_create_queue(const char* name);
void packet_sched_destroy_queue(sched_queue_t* queue);

/* Enqueue/dequeue packets */
int packet_sched_enqueue(sched_queue_t* queue, sk_buff_t* skb);
sk_buff_t* packet_sched_dequeue(sched_queue_t* queue);

/* Get statistics */
void packet_sched_get_stats(sched_queue_t* queue, u64* packets, u64* bytes, u64* processed);

#endif /* PACKET_SCHED_H */
