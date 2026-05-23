#include "syscall.h"
#include "tcp.h"
#include "net.h"
#include "fs/vfs.h"
#include "epoll.h"

extern socket_t* syscall_fd_get_socket(u64 fd);

int socket_fd_poll_events(int fd) {
    if (fd < 3) {
        return 0;
    }
    int vfs_ev = vfs_fd_poll_events(fd);
    if (vfs_ev) {
        return vfs_ev;
    }
    socket_t* sock = syscall_fd_get_socket((u64)fd);
    if (sock) {
        return tcp_socket_poll_events(sock);
    }
    return 0;
}
