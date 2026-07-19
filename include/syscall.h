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
#define SYS_SOCKET    11
#define SYS_BIND      12
#define SYS_CONNECT   13
#define SYS_LISTEN    14
#define SYS_ACCEPT    15
#define SYS_SEND      16
#define SYS_RECV      17
#define SYS_SENDTO    18
#define SYS_CLOSEFD   19
#define SYS_DNS       20
#define SYS_AI_METRICS 21
#define SYS_EPOLL_CREATE 22
#define SYS_EPOLL_CTL    23
#define SYS_EPOLL_WAIT   24
#define SYS_POLL         25
#define SYS_GETPID       26
#define SYS_CLOCK_GETTIME 27
#define SYS_BRK          28
#define SYS_MPROTECT     29
#define SYS_RT_SIGACTION   30
#define SYS_RT_SIGPROCMASK 31
#define SYS_KILL           32
#define SYS_LSEEK          33
#define SYS_MKDIR          34
#define SYS_UNLINK         35
#define SYS_RMDIR          36
#define SYS_GETDENTS       37
#define SYS_PIPE           38
#define SYS_DUP            39
#define SYS_DUP2           40
#define SYS_STAT           41
#define SYS_FTRUNCATE      42

/* lseek whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* mmap prot / flags (subset, values match Linux). */
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

/* System call handler (uframe = saved-register block on the kernel stack). */
u64 syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5,
                    void* uframe);

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
u64 sys_wait(u64 pid, int* status);
u64 sys_mmap(void* addr, u64 length, u64 prot, u64 flags);
u64 sys_munmap(void* addr, u64 length);
u64 sys_sigreturn(void);
u64 sys_rt_sigaction(u64 sig, const void* act, void* oldact);
u64 sys_rt_sigprocmask(u64 how, const void* set, void* oldset);
u64 sys_kill(u64 pid, u64 sig);

void syscall_socket_init(void);
u64 sys_socket(u64 domain, u64 type, u64 protocol);
u64 sys_bind(u64 fd, const void* addr, u64 addrlen);
u64 sys_connect(u64 fd, const void* addr, u64 addrlen);
u64 sys_listen(u64 fd, u64 backlog);
u64 sys_accept(u64 fd, void* addr, u64 addrlen);
u64 sys_send(u64 fd, const void* buf, u64 len, u64 flags);
u64 sys_recv(u64 fd, void* buf, u64 len, u64 flags);
u64 sys_sendto(u64 fd, const void* buf, u64 len, u64 flags,
               const void* addr, u64 addrlen);
u64 sys_socket_close(u64 fd);
u64 sys_dns_resolve(const char* hostname, void* out_ip, u64 out_len);
u64 sys_ai_metrics(void* out_info, u64 size);
u64 sys_epoll_create(u64 size);
u64 sys_epoll_ctl(u64 epfd, u64 op, u64 fd, void* event);
u64 sys_epoll_wait(u64 epfd, void* events, u64 maxevents, u64 timeout);
u64 sys_poll(void* fds, u64 nfds, u64 timeout_ms);
u64 sys_getpid(void);
u64 sys_clock_gettime(u64 clk_id, void* tp);
u64 sys_brk(u64 new_brk);
u64 sys_mprotect(void* addr, u64 length, u64 prot);

/* File / directory syscalls (P5). */
u64 sys_lseek(u64 fd, i64 offset, u64 whence);
u64 sys_mkdir(const char* path, u64 mode);
u64 sys_unlink(const char* path);
u64 sys_rmdir(const char* path);
u64 sys_getdents(u64 fd, void* dirp, u64 count);
u64 sys_pipe(int* pipefd);
u64 sys_dup(u64 oldfd);
u64 sys_dup2(u64 oldfd, u64 newfd);
u64 sys_stat(const char* path, void* statbuf);
u64 sys_ftruncate(u64 fd, u64 length);

/* Linux-like dirent for getdents. */
typedef struct linux_dirent64_k {
    u64 d_ino;
    i64 d_off;
    u16 d_reclen;
    u8  d_type;
    char d_name[];
} linux_dirent64_k_t;

/* Minimal stat exposed to user (subset). */
typedef struct user_stat {
    u64 st_ino;
    u32 st_mode;
    u64 st_size;
} user_stat_t;

#endif /* SYSCALL_H */
