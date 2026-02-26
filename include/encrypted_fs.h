#ifndef ENCRYPTED_FS_H
#define ENCRYPTED_FS_H

#include "types.h"
#include "fs/vfs.h"

/* Encryption algorithms */
#define ENCRYPT_ALGO_AES_256_XTS 0
#define ENCRYPT_ALGO_AES_128_CBC 1
#define ENCRYPT_ALGO_AES_256_CBC 2

/* Encryption key structure */
typedef struct encrypt_key {
    u8 key[64];
    u32 key_size;
    u32 algorithm;
} encrypt_key_t;

/* Initialize encrypted filesystem */
int encrypted_fs_init(void);

/* Mount encrypted filesystem */
int encrypted_fs_mount(const char* source, const char* target, const encrypt_key_t* key);

/* Unmount encrypted filesystem */
int encrypted_fs_unmount(const char* target);

/* Change encryption key */
int encrypted_fs_change_key(const char* path, const encrypt_key_t* old_key, const encrypt_key_t* new_key);

/* Encrypt data */
int encrypted_fs_encrypt(const void* plaintext, size_t len, void* ciphertext, const encrypt_key_t* key);

/* Decrypt data */
int encrypted_fs_decrypt(const void* ciphertext, size_t len, void* plaintext, const encrypt_key_t* key);

#endif /* ENCRYPTED_FS_H */
