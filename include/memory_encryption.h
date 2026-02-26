#ifndef MEMORY_ENCRYPTION_H
#define MEMORY_ENCRYPTION_H

#include "types.h"

/* Memory encryption types */
#define MEM_ENCRYPT_NONE    0
#define MEM_ENCRYPT_SME     1  /* AMD Secure Memory Encryption */
#define MEM_ENCRYPT_TME     2  /* Intel Total Memory Encryption */

/* Initialize memory encryption */
void memory_encryption_init(void);

/* Enable memory encryption */
int memory_encryption_enable(u32 type);

/* Encrypt memory region */
int memory_encrypt_region(void* addr, size_t size);

/* Decrypt memory region */
int memory_decrypt_region(void* addr, size_t size);

/* Check if encryption enabled */
bool memory_encryption_is_enabled(void);

#endif /* MEMORY_ENCRYPTION_H */
