#ifndef USERFAULTFD_H
#define USERFAULTFD_H

#include "types.h"

/* Userfaultfd flags */
#define UFFD_CLOEXEC    0x00000001
#define UFFD_NONBLOCK   0x00000002

/* Userfaultfd ioctl commands */
#define UFFDIO_API      0xC018AA00
#define UFFDIO_REGISTER 0xC018AA01
#define UFFDIO_UNREGISTER 0xC018AA02
#define UFFDIO_WAKE     0xC018AA03
#define UFFDIO_COPY     0xC018AA04
#define UFFDIO_ZEROPAGE 0xC018AA05

/* Userfaultfd API structure */
typedef struct uffdio_api {
    u64 api;
    u64 features;
    u64 ioctls;
} uffdio_api_t;

/* Userfaultfd range structure */
typedef struct uffdio_register {
    void* addr;
    u64 len;
    u64 mode;
    u64 ioctls;
} uffdio_register_t;

/* Userfaultfd copy structure */
typedef struct uffdio_copy {
    void* dst;
    void* src;
    u64 len;
    u64 mode;
    u64 copy;
} uffdio_copy_t;

/* Userfaultfd event structure */
typedef struct uffd_msg {
    u8 event;
    u8 reserved1;
    u16 reserved2;
    u32 reserved3;
    union {
        struct {
            u64 flags;
            u64 address;
            u32 ptid;
        } pagefault;
        struct {
            u32 ufd;
        } fork;
        struct {
            u32 ufd;
        } remap;
        struct {
            u64 from;
            u64 to;
            u64 len;
        } remove;
    } arg;
} uffd_msg_t;

/* Create userfaultfd */
i32 userfaultfd(i32 flags);

/* Read from userfaultfd */
ssize_t userfaultfd_read(i32 fd, uffd_msg_t* msg);

/* Write to userfaultfd (wake) */
ssize_t userfaultfd_write(i32 fd, const void* buf, size_t count);

#endif /* USERFAULTFD_H */
