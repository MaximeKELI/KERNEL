#include "ima.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "secure_random.h"
#include "fs/inode.h"

#define MAX_MEASUREMENTS 4096

static ima_measurement_t* measurement_list = NULL;
static u32 ima_mode = IMA_MEASURE_FILE | IMA_MEASURE_BPRM;
static u32 ima_hash_algo = IMA_HASH_SHA256;
static spinlock_t ima_lock = SPINLOCK_INIT;

static void ima_hash(const void* data, size_t size, u8* hash, u32* hash_size) {
    /* Simplified hash - would use actual SHA */
    u32 algo_size = 32; /* SHA256 */
    if (ima_hash_algo == IMA_HASH_SHA1) algo_size = 20;
    else if (ima_hash_algo == IMA_HASH_SHA512) algo_size = 64;
    
    /* Would compute actual hash */
    secure_random_bytes(hash, algo_size);
    
    *hash_size = algo_size;
    
    (void)data;
    (void)size;
}

int ima_init(void) {
    ima_mode = IMA_MEASURE_FILE | IMA_MEASURE_BPRM;
    DEBUG_INFO("%s", "IMA (Integrity Measurement Architecture) initialized");
    return 0;
}

int ima_measure_file(const char* path, const void* data, size_t size) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(data, -1);
    
    if (!(ima_mode & IMA_MEASURE_FILE)) {
        return 0; /* Measurement disabled */
    }
    
    spinlock_lock(&ima_lock);
    
    /* Create measurement entry */
    ima_measurement_t* entry = (ima_measurement_t*)kmalloc(sizeof(ima_measurement_t));
    if (!entry) {
        spinlock_unlock(&ima_lock);
        return -1;
    }
    
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    entry->path[sizeof(entry->path) - 1] = '\0';
    entry->inode = 0; /* Would get from path */
    entry->size = size;
    entry->timestamp = 0; /* Would get current time */
    
    ima_hash(data, size, entry->hash, &entry->hash_size);
    
    entry->next = measurement_list;
    measurement_list = entry;
    
    spinlock_unlock(&ima_lock);
    
    DEBUG_INFO("IMA measurement: path=%s, size=%u", path, (u32)size);
    return 0;
}

ima_measurement_t* ima_get_measurements(void) {
    return measurement_list;
}

int ima_verify_file(const char* path, const void* data, size_t size) {
    VALIDATE_STRING(path, 256);
    VALIDATE_PTR_RET(data, -1);
    
    spinlock_lock(&ima_lock);
    
    /* Find measurement */
    ima_measurement_t* entry = measurement_list;
    while (entry) {
        if (strcmp(entry->path, path) == 0 && entry->size == size) {
            /* Compute current hash */
            u8 current_hash[64];
            u32 hash_size;
            ima_hash(data, size, current_hash, &hash_size);
            
            /* Compare */
            bool match = (memcmp(entry->hash, current_hash, hash_size) == 0);
            
            spinlock_unlock(&ima_lock);
            
            if (!match) {
                DEBUG_ERROR("IMA verification failed: path=%s", path);
                return -1;
            }
            
            return 0;
        }
        entry = entry->next;
    }
    
    spinlock_unlock(&ima_lock);
    
    /* No measurement found - would allow or deny based on policy */
    return 0;
}

int ima_set_mode(u32 mode) {
    spinlock_lock(&ima_lock);
    ima_mode = mode;
    spinlock_unlock(&ima_lock);
    
    DEBUG_INFO("IMA mode set: 0x%x", mode);
    return 0;
}
