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

/* System call handler */
u64 syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

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
u64 sys_sigreturn(void);

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

#endif /* SYSCALL_H */
