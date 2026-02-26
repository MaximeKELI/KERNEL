#ifndef EVM_H
#define EVM_H

#include "types.h"

/* EVM modes */
#define EVM_MODE_DISABLED  0
#define EVM_MODE_ENABLED   1
#define EVM_MODE_FIXED     2

/* EVM hash */
typedef struct evm_hash {
    u8 hash[64];
    u32 hash_size;
} evm_hash_t;

/* Initialize EVM */
int evm_init(void);

/* Calculate EVM hash */
int evm_calc_hash(const char* path, const void* data, size_t size, evm_hash_t* hash);

/* Verify EVM hash */
int evm_verify_hash(const char* path, const void* data, size_t size, const evm_hash_t* expected);

/* Set EVM mode */
int evm_set_mode(u32 mode);

/* Get EVM mode */
u32 evm_get_mode(void);

#endif /* EVM_H */
