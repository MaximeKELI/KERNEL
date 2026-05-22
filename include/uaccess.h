#ifndef UACCESS_H
#define UACCESS_H

#include "types.h"

#define USER_ADDR_MIN 0x400000UL
#define USER_ADDR_MAX 0x7FFFFFFFUL

bool user_ptr_ok(const void* ptr, size_t len);
ssize_t copy_from_user(void* kernel_dst, const void* user_src, size_t len);
ssize_t copy_to_user(void* user_dst, const void* kernel_src, size_t len);

#endif /* UACCESS_H */
