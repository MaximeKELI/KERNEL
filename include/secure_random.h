#ifndef SECURE_RANDOM_H
#define SECURE_RANDOM_H

#include "types.h"

/* Initialize secure random */
void secure_random_init(void);

/* Get secure random bytes */
int secure_random_get_bytes(u8* buffer, size_t size);

/* Get secure random u32 */
u32 secure_random_u32(void);

/* Get secure random u64 */
u64 secure_random_u64(void);

/* Add entropy */
void secure_random_add_entropy(const u8* data, size_t size);

#endif /* SECURE_RANDOM_H */
