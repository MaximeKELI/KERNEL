#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

/* System call numbers */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_FORK    5
#define SYS_EXEC    6
#define SYS_WAIT    7
#define SYS_MMAP    8
#define SYS_MUNMAP  9
#define SYS_SIGRETURN 10

/* System call handler */
void syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

/* Initialize syscall subsystem */
void syscall_init(void);

/* System call implementations */
u64 sys_exit(u64 status);
u64 sys_write(u64 fd, const void* buf, u64 count);
u64 sys_read(u64 fd, void* buf, u64 count);
u64 sys_open(const char* path, u64 flags);
u64 sys_close(u64 fd);
u64 sys_fork(void);
u64 sys_exec(const char* path, char* const argv[]);
u64 sys_wait(u64 pid);
u64 sys_mmap(void* addr, u64 length, u64 prot, u64 flags);
u64 sys_munmap(void* addr, u64 length);

#endif /* SYSCALL_H */
