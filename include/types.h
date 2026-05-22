#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Standard integer types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef i64      ssize_t;

/* Size types */
typedef uintptr_t uintptr;
typedef intptr_t  intptr;

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Boolean */
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif

/* Attributes */
#define __packed __attribute__((packed))
#define __aligned(x) __attribute__((aligned(x)))
#define __noreturn __attribute__((noreturn))
#define __unused __attribute__((unused))

/* Memory */
#define PAGE_SIZE 4096
#define PAGE_SIZE_2MB (2 * 1024 * 1024)
#define PAGE_SIZE_1GB (1024 * 1024 * 1024)

/* Address manipulation */
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

#endif /* TYPES_H */
