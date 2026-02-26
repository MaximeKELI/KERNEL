#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"
#include "stdio.h"

/* Debug levels */
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_VERBOSE 4

/* Current debug level */
extern u32 debug_level;

/* Debug macros */
#define DEBUG_ERROR(fmt, ...) \
    do { if (debug_level >= DEBUG_LEVEL_ERROR) printk("[ERROR] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_WARN(fmt, ...) \
    do { if (debug_level >= DEBUG_LEVEL_WARN) printk("[WARN] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_INFO(fmt, ...) \
    do { if (debug_level >= DEBUG_LEVEL_INFO) printk("[INFO] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_VERBOSE(fmt, ...) \
    do { if (debug_level >= DEBUG_LEVEL_VERBOSE) printk("[VERBOSE] " fmt "\n", ##__VA_ARGS__); } while(0)

/* Assertions */
#ifdef NDEBUG
#define ASSERT(condition) ((void)0)
#else
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printk("[ASSERT FAILED] %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            panic("Assertion failed"); \
        } \
    } while(0)
#endif

/* Memory checks */
#define CHECK_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            DEBUG_ERROR("Null pointer at %s:%d", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define CHECK_PTR_RET(ptr, ret) \
    do { \
        if ((ptr) == NULL) { \
            DEBUG_ERROR("Null pointer at %s:%d", __FILE__, __LINE__); \
            return (ret); \
        } \
    } while(0)

/* Set debug level */
void debug_set_level(u32 level);

#endif /* DEBUG_H */
