#ifndef VALIDATE_H
#define VALIDATE_H

#include "types.h"
#include "debug.h"
#include "kernel.h"

/* Validation macros */
#define VALIDATE_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            DEBUG_ERROR("Null pointer at %s:%d", __FILE__, __LINE__); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_PTR_VOID(ptr) \
    do { \
        if ((ptr) == NULL) { \
            DEBUG_ERROR("Null pointer at %s:%d", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define VALIDATE_PTR_RET(ptr, ret) \
    do { \
        if ((ptr) == NULL) { \
            DEBUG_ERROR("Null pointer at %s:%d", __FILE__, __LINE__); \
            return (ret); \
        } \
    } while(0)

#define VALIDATE_SIZE(size) \
    do { \
        if ((size) == 0) { \
            DEBUG_ERROR("Invalid size (0) at %s:%d", __FILE__, __LINE__); \
            return -1; \
        } \
        if ((size) > (1024 * 1024 * 1024)) { \
            DEBUG_ERROR("Size too large at %s:%d: %u", __FILE__, __LINE__, (u32)(size)); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_SIZE_RET_NULL(size) \
    do { \
        if ((size) == 0) { \
            DEBUG_ERROR("Invalid size (0) at %s:%d", __FILE__, __LINE__); \
            return NULL; \
        } \
        if ((size) > (1024 * 1024 * 1024)) { \
            DEBUG_ERROR("Size too large at %s:%d: %u", __FILE__, __LINE__, (u32)(size)); \
            return NULL; \
        } \
    } while(0)

#define VALIDATE_RANGE(val, min, max) \
    do { \
        if ((val) < (min) || (val) > (max)) { \
            DEBUG_ERROR("Value out of range at %s:%d: %lld not in [%lld, %lld]", \
                       __FILE__, __LINE__, (long long)(val), (long long)(min), (long long)(max)); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_STRING(str, max_len) \
    do { \
        if ((str) == NULL) { \
            DEBUG_ERROR("Null string at %s:%d", __FILE__, __LINE__); \
            return -1; \
        } \
        if (strlen((str)) > (max_len)) { \
            DEBUG_ERROR("String too long at %s:%d: %u > %u", \
                       __FILE__, __LINE__, (u32)strlen((str)), (u32)(max_len)); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_INDEX(idx, max) \
    do { \
        if ((idx) >= (max)) { \
            DEBUG_ERROR("Index out of bounds at %s:%d: %u >= %u", \
                       __FILE__, __LINE__, (u32)(idx), (u32)(max)); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_FLAGS(flags, valid_mask) \
    do { \
        if (((flags) & ~(valid_mask)) != 0) { \
            DEBUG_ERROR("Invalid flags at %s:%d: 0x%x", \
                       __FILE__, __LINE__, (u32)(flags)); \
            return -1; \
        } \
    } while(0)

/* Assertion macros (panic on failure) */
#define ASSERT_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            panic("Assertion failed: NULL pointer at " __FILE__ ":" __LINE__); \
        } \
    } while(0)

#define ASSERT_COND(condition) \
    do { \
        if (!(condition)) { \
            panic("Assertion failed: " #condition " at " __FILE__ ":" __LINE__); \
        } \
    } while(0)

#define ASSERT_RANGE(val, min, max) \
    do { \
        if ((val) < (min) || (val) > (max)) { \
            panic("Assertion failed: value out of range at " __FILE__ ":" __LINE__); \
        } \
    } while(0)

#endif /* VALIDATE_H */
