#include "syscall.h"
#include "syscall_socket.h"
#include "tcp.h"
#include "net.h"
#include "fs/vfs.h"

extern socket_t* syscall_fd_get_socket(u64 fd);

int socket_fd_poll_events(int fd) {
    if (fd < 3) {
        return 0;
    }
    socket_t* sock = syscall_fd_get_socket((u64)fd);
    if (sock) {
        return tcp_socket_poll_events(sock);
    }
    return 0;
}
