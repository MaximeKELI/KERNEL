#ifndef ABI_ERRNO_H
#define ABI_ERRNO_H

/* Linux errno subset. Kernel syscalls return -errno on failure, per the
 * x86-64 System V / Linux convention. */
#define EPERM    1
#define ENOENT   2
#define ESRCH    3
#define EINTR    4
#define EIO      5
#define EBADF    9
#define ECHILD   10
#define EAGAIN   11
#define ENOMEM   12
#define EACCES   13
#define EFAULT   14
#define EEXIST   17
#define ENOTDIR  20
#define EISDIR   21
#define EINVAL   22
#define ENFILE   23
#define EMFILE   24
#define ENOSPC   28
#define ESPIPE   29
#define EROFS    30
#define ERANGE   34
#define ENOSYS   38
#define ENOTEMPTY 39

#endif /* ABI_ERRNO_H */
