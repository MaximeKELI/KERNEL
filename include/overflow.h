#ifndef OVERFLOW_H
#define OVERFLOW_H

#include "types.h"
#include "debug.h"

/* Check for addition overflow */
#define CHECK_ADD_OVERFLOW(a, b, result) \
    do { \
        if ((b) > 0 && (a) > SIZE_MAX - (b)) { \
            DEBUG_ERROR("Addition overflow at %s:%d: %llu + %llu", \
                       __FILE__, __LINE__, (unsigned long long)(a), (unsigned long long)(b)); \
            return -1; \
        } \
        if ((b) < 0 && (a) < (size_t)(-(b))) { \
            DEBUG_ERROR("Addition underflow at %s:%d", __FILE__, __LINE__); \
            return -1; \
        } \
        *(result) = (a) + (b); \
    } while(0)

/* Check for multiplication overflow */
#define CHECK_MUL_OVERFLOW(a, b, result) \
    do { \
        if ((b) != 0 && (a) > SIZE_MAX / (b)) { \
            DEBUG_ERROR("Multiplication overflow at %s:%d: %llu * %llu", \
                       __FILE__, __LINE__, (unsigned long long)(a), (unsigned long long)(b)); \
            return -1; \
        } \
        *(result) = (a) * (b); \
    } while(0)

/* Check for subtraction underflow */
#define CHECK_SUB_OVERFLOW(a, b, result) \
    do { \
        if ((a) < (b)) { \
            DEBUG_ERROR("Subtraction underflow at %s:%d: %llu - %llu", \
                       __FILE__, __LINE__, (unsigned long long)(a), (unsigned long long)(b)); \
            return -1; \
        } \
        *(result) = (a) - (b); \
    } while(0)

/* Safe addition */
static inline size_t safe_add(size_t a, size_t b, int* overflow) {
    if (b > SIZE_MAX - a) {
        if (overflow) *overflow = 1;
        return SIZE_MAX;
    }
    if (overflow) *overflow = 0;
    return a + b;
}

/* Safe multiplication */
static inline size_t safe_mul(size_t a, size_t b, int* overflow) {
    if (b != 0 && a > SIZE_MAX / b) {
        if (overflow) *overflow = 1;
        return SIZE_MAX;
    }
    if (overflow) *overflow = 0;
    return a * b;
}

/* Safe subtraction */
static inline size_t safe_sub(size_t a, size_t b, int* underflow) {
    if (a < b) {
        if (underflow) *underflow = 1;
        return 0;
    }
    if (underflow) *underflow = 0;
    return a - b;
}

#endif /* OVERFLOW_H */
