#ifndef IMA_H
#define IMA_H

#include "types.h"

/* IMA measurement modes */
#define IMA_MEASURE_NONE    0
#define IMA_MEASURE_FILE    1
#define IMA_MEASURE_BPRM    2
#define IMA_MEASURE_MODULE  4
#define IMA_MEASURE_FIRMWARE 8

/* IMA hash algorithms */
#define IMA_HASH_SHA1   0
#define IMA_HASH_SHA256 1
#define IMA_HASH_SHA512 2

/* IMA measurement entry */
typedef struct ima_measurement {
    u8 hash[64];
    u32 hash_size;
    char path[256];
    u64 inode;
    u64 size;
    u64 timestamp;
    struct ima_measurement* next;
} ima_measurement_t;

/* Initialize IMA */
int ima_init(void);

/* Measure file */
int ima_measure_file(const char* path, const void* data, size_t size);

/* Get measurement list */
ima_measurement_t* ima_get_measurements(void);

/* Verify file integrity */
int ima_verify_file(const char* path, const void* data, size_t size);

/* Set measurement mode */
int ima_set_mode(u32 mode);

#endif /* IMA_H */
