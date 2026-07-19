#ifndef ABI_LINUX_SYSCALL_H
#define ABI_LINUX_SYSCALL_H

/* x86_64 Linux syscall numbers (musl/glibc compatible subset) */
#define __NR_read          0
#define __NR_write         1
#define __NR_open          2
#define __NR_close         3
#define __NR_stat          4
#define __NR_fstat         5
#define __NR_lseek         8
#define __NR_mmap          9
#define __NR_mprotect      10
#define __NR_munmap        11
#define __NR_brk           12
#define __NR_rt_sigaction  13
#define __NR_rt_sigprocmask 14
#define __NR_rt_sigreturn  15
#define __NR_ioctl         16
#define __NR_writev        20
#define __NR_pipe          22
#define __NR_dup           32
#define __NR_dup2          33
#define __NR_nanosleep     35
#define __NR_getpid        39
#define __NR_socket        41
#define __NR_connect       42
#define __NR_accept        43
#define __NR_sendto        44
#define __NR_recvfrom      45
#define __NR_bind          49
#define __NR_listen        50
#define __NR_clone         56
#define __NR_fork          57
#define __NR_execve        59
#define __NR_exit          60
#define __NR_wait4         61
#define __NR_kill          62
#define __NR_uname         63
#define __NR_getcwd        79
#define __NR_chdir         80
#define __NR_mkdir         83
#define __NR_rmdir         84
#define __NR_unlink        87
#define __NR_ftruncate     77
#define __NR_getdents64    217
#define __NR_arch_prctl    158
#define __NR_gettid        186
#define __NR_poll          7
#define __NR_clock_gettime 228
#define __NR_epoll_wait    232
#define __NR_epoll_ctl     233
#define __NR_set_tid_address 218
#define __NR_exit_group    231
#define __NR_epoll_create1 291

/* arch_prctl codes */
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_SET_GS 0x1001
#define ARCH_GET_GS 0x1004

/* This kernel's syscall ABI (internal nettest) */
#define K_SYS_EXIT         0
#define K_SYS_WRITE        1
#define K_SYS_READ         2
#define K_SYS_OPEN         3
#define K_SYS_CLOSE        4
#define K_SYS_FORK         5
#define K_SYS_EXEC         6
#define K_SYS_WAIT         7
#define K_SYS_EPOLL_CREATE 22
#define K_SYS_EPOLL_CTL    23
#define K_SYS_EPOLL_WAIT   24
#define K_SYS_POLL         25
#define K_SYS_GETPID       26
#define K_SYS_CLOCK_GETTIME 27

#endif /* ABI_LINUX_SYSCALL_H */
