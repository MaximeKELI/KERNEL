#ifndef BLK_MQ_H
#define BLK_MQ_H

#include "types.h"
#include "block.h"

void blk_mq_init(void);
u32 blk_mq_queue_count(void);
int blk_mq_submit(bio_t* bio, u32 queue_id);
u32 blk_mq_dispatch(u32 queue_id);
void blk_mq_get_stats(u32 queue_id, u64* submitted, u64* completed);

#endif /* BLK_MQ_H */
