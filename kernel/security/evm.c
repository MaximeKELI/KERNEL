#include "evm.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "secure_random.h"
#include "xattr.h"

static u32 evm_mode = EVM_MODE_DISABLED;
static spinlock_t evm_lock = SPINLOCK_INIT;

static void evm_compute_hash(const char* path, const void* data, size_t size, 
                            const void* xattrs, size_t xattr_size,
                            u8* hash, u32* hash_size) {
    /* EVM hash = HMAC(data + xattrs + path) */
    /* Simplified - would use actual HMAC */
    
    u32 algo_size = 32; /* SHA256 HMAC */
    secure_random_get_bytes(hash, algo_size);
    
    *hash_size = algo_size;
    
    (void)path;
    (void)data;
    (void)size;
    (void)xattrs;
    (void)xattr_size;
}

int evm_init(void) {
    evm_mode = EVM_MODE_DISABLED;
    DEBUG_INFO("%s", "EVM (Extended Verification Module) initialized");
    return 0;
}

int evm_calc_hash(const char* path, const void* data, size_t size, evm_hash_t* hash) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(data, -1);
    VALIDATE_PTR_RET(hash, -1);
    
    if (evm_mode == EVM_MODE_DISABLED) {
        return -1;
    }
    
    /* Get extended attributes */
    char xattr_names[4096];
    ssize_t xattr_size = xattr_list(path, xattr_names, sizeof(xattr_names));
    
    /* Compute EVM hash */
    evm_compute_hash(path, data, size, xattr_names, (size_t)xattr_size, 
                     hash->hash, &hash->hash_size);
    
    DEBUG_INFO("EVM hash calculated: path=%s", path);
    return 0;
}

int evm_verify_hash(const char* path, const void* data, size_t size, const evm_hash_t* expected) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(data, -1);
    VALIDATE_PTR_RET(expected, -1);
    
    if (evm_mode == EVM_MODE_DISABLED) {
        return 0; /* Verification disabled */
    }
    
    evm_hash_t current;
    if (evm_calc_hash(path, data, size, &current) != 0) {
        return -1;
    }
    
    if (current.hash_size != expected->hash_size ||
        memcmp(current.hash, expected->hash, current.hash_size) != 0) {
        DEBUG_ERROR("EVM verification failed: path=%s", path);
        return -1;
    }
    
    return 0;
}

int evm_set_mode(u32 mode) {
    if (mode > EVM_MODE_FIXED) {
        return -1;
    }
    
    spinlock_lock(&evm_lock);
    evm_mode = mode;
    spinlock_unlock(&evm_lock);
    
    DEBUG_INFO("EVM mode set: %u", mode);
    return 0;
}

u32 evm_get_mode(void) {
    return evm_mode;
}
