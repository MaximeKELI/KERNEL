#include "uaccess.h"
#include "string.h"

bool user_ptr_ok(const void* ptr, size_t len) {
    if (!ptr || len == 0) {
        return false;
    }
    uintptr_t start = (uintptr_t)ptr;
    uintptr_t end = start + len - 1;
    if (start < USER_ADDR_MIN || end > USER_ADDR_MAX || end < start) {
        return false;
    }
    return true;
}

ssize_t copy_from_user(void* kernel_dst, const void* user_src, size_t len) {
    if (!kernel_dst || !user_ptr_ok(user_src, len)) {
        return -1;
    }
    memcpy(kernel_dst, user_src, len);
    return (ssize_t)len;
}

ssize_t copy_to_user(void* user_dst, const void* kernel_src, size_t len) {
    if (!kernel_src || !user_ptr_ok(user_dst, len)) {
        return -1;
    }
    memcpy(user_dst, kernel_src, len);
    return (ssize_t)len;
}
