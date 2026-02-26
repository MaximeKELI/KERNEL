#ifndef MPK_H
#define MPK_H

#include "types.h"

/* Memory Protection Keys */
#define MPK_KEYS_MAX 16

/* MPK protection flags */
#define MPK_PROT_READ    0x01
#define MPK_PROT_WRITE   0x02
#define MPK_PROT_EXEC    0x04

/* MPK operations */
#define MPK_ALLOC_KEY    0
#define MPK_FREE_KEY     1
#define MPK_SET_PROT     2
#define MPK_GET_PROT     3

/* Initialize MPK */
void mpk_init(void);

/* Allocate protection key */
int mpk_alloc_key(void);

/* Free protection key */
int mpk_free_key(u32 key);

/* Set protection for address range */
int mpk_set_prot(void* addr, size_t len, u32 key, u32 prot);

/* Get protection for address */
u32 mpk_get_prot(void* addr, u32* key);

#endif /* MPK_H */
