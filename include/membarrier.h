#ifndef MEMBARRIER_H
#define MEMBARRIER_H

#include "types.h"

/* Membarrier commands */
#define MEMBARRIER_CMD_QUERY           0
#define MEMBARRIER_CMD_GLOBAL          1
#define MEMBARRIER_CMD_GLOBAL_EXPEDITED 2
#define MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED 4
#define MEMBARRIER_CMD_PRIVATE_EXPEDITED 8
#define MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED 16
#define MEMBARRIER_CMD_PRIVATE_EXPEDITED_SYNC_CORE 32
#define MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED_SYNC_CORE 64

/* Membarrier system call */
int membarrier(i32 cmd, i32 flags);

/* Memory barrier operations */
void mb(void);
void rmb(void);
void wmb(void);
void smp_mb(void);
void smp_rmb(void);
void smp_wmb(void);

#endif /* MEMBARRIER_H */
