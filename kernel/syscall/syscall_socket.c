#include "syscall.h"
#include "net.h"
#include "net_socket.h"
#include "ip.h"
#include "udp.h"
#include "uaccess.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

#define MAX_FDS 64
#define FD_SOCKET_BASE 3

typedef struct {
    bool used;
    socket_t* sock;
} fd_entry_t;

static fd_entry_t fd_table[MAX_FDS];

static int fd_alloc(socket_t* sock) {
    for (int i = FD_SOCKET_BASE; i < MAX_FDS; i++) {
        if (!fd_table[i].used) {
            fd_table[i].used = true;
            fd_table[i].sock = sock;
            return i;
        }
    }
    return -1;
}

static socket_t* fd_get(u64 fd) {
    if (fd >= MAX_FDS) {
        return NULL;
    }
    if (!fd_table[fd].used) {
        return NULL;
    }
    return fd_table[fd].sock;
}

socket_t* syscall_fd_get_socket(u64 fd) {
    return fd_get(fd);
}

static void fd_free(u64 fd) {
    if (fd >= MAX_FDS || !fd_table[fd].used) {
        return;
    }
    if (fd_table[fd].sock) {
        if (fd_table[fd].sock->ops && fd_table[fd].sock->ops->close) {
            fd_table[fd].sock->ops->close(fd_table[fd].sock);
        }
        socket_destroy(fd_table[fd].sock);
    }
    fd_table[fd].used = false;
    fd_table[fd].sock = NULL;
}

void syscall_socket_init(void) {
    memset(fd_table, 0, sizeof(fd_table));
}

u64 sys_socket(u64 domain, u64 type, u64 protocol) {
    socket_t* sock = socket_create((int)domain, (int)type, (int)protocol);
    if (!sock) {
        return (u64)-1;
    }
    int fd = fd_alloc(sock);
    if (fd < 0) {
        socket_destroy(sock);
        return (u64)-1;
    }
    return (u64)fd;
}

u64 sys_bind(u64 fd, const void* addr, u64 addrlen) {
    socket_t* sock = fd_get(fd);
    if (!sock || !sock->ops || !sock->ops->bind) {
        return (u64)-1;
    }
    sockaddr_t kaddr;
    if (addrlen < sizeof(sockaddr_t) || copy_from_user(&kaddr, addr, sizeof(sockaddr_t)) < 0) {
        return (u64)-1;
    }
    return sock->ops->bind(sock, &kaddr) == 0 ? 0 : (u64)-1;
}

u64 sys_connect(u64 fd, const void* addr, u64 addrlen) {
    socket_t* sock = fd_get(fd);
    if (!sock || !sock->ops || !sock->ops->connect) {
        return (u64)-1;
    }
    sockaddr_t kaddr;
    if (copy_from_user(&kaddr, addr, sizeof(sockaddr_t)) < 0) {
        return (u64)-1;
    }
    return sock->ops->connect(sock, &kaddr) == 0 ? 0 : (u64)-1;
}

u64 sys_listen(u64 fd, u64 backlog) {
    socket_t* sock = fd_get(fd);
    if (!sock || !sock->ops || !sock->ops->listen) {
        return (u64)-1;
    }
    return sock->ops->listen(sock, (int)backlog) == 0 ? 0 : (u64)-1;
}

u64 sys_accept(u64 fd, void* addr, u64 addrlen) {
    socket_t* sock = fd_get(fd);
    if (!sock || !sock->ops || !sock->ops->accept) {
        return (u64)-1;
    }
    sockaddr_t kaddr;
    socket_t* newsock = sock->ops->accept(sock, addr ? &kaddr : NULL);
    if (!newsock) {
        return (u64)-1;
    }
    if (addr && addrlen >= sizeof(sockaddr_t)) {
        copy_to_user(addr, &kaddr, sizeof(sockaddr_t));
    }
    int nfd = fd_alloc(newsock);
    if (nfd < 0) {
        socket_destroy(newsock);
        return (u64)-1;
    }
    return (u64)nfd;
}

u64 sys_send(u64 fd, const void* buf, u64 len, u64 flags) {
    (void)flags;
    socket_t* sock = fd_get(fd);
    if (!sock || !buf || !sock->ops || !sock->ops->send) {
        return (u64)-1;
    }
    u8* kbuf = (u8*)kmalloc((size_t)len);
    if (!kbuf) {
        return (u64)-1;
    }
    if (copy_from_user(kbuf, buf, (size_t)len) < 0) {
        kfree(kbuf);
        return (u64)-1;
    }
    ssize_t n = sock->ops->send(sock, kbuf, (size_t)len);
    kfree(kbuf);
    return n < 0 ? (u64)-1 : (u64)n;
}

u64 sys_recv(u64 fd, void* buf, u64 len, u64 flags) {
    (void)flags;
    socket_t* sock = fd_get(fd);
    if (!sock || !buf || !sock->ops || !sock->ops->recv) {
        return (u64)-1;
    }
    u8* kbuf = (u8*)kmalloc((size_t)len);
    if (!kbuf) {
        return (u64)-1;
    }
    ssize_t n = sock->ops->recv(sock, kbuf, (size_t)len);
    if (n > 0) {
        copy_to_user(buf, kbuf, (size_t)n);
    }
    kfree(kbuf);
    return n < 0 ? (u64)-1 : (u64)n;
}

u64 sys_sendto(u64 fd, const void* buf, u64 len, u64 flags,
               const void* addr, u64 addrlen) {
    (void)flags;
    (void)addrlen;
    socket_t* sock = fd_get(fd);
    if (!sock || !buf || sock->type != SOCK_DGRAM) {
        return (u64)-1;
    }
    sockaddr_t kaddr;
    if (copy_from_user(&kaddr, addr, sizeof(sockaddr_t)) < 0) {
        return (u64)-1;
    }
    ip_addr_t dst;
    u16 port;
    memcpy(&port, &kaddr.sa_data[0], 2);
    port = ntohs(port);
    memcpy(&dst, &kaddr.sa_data[2], 4);
    u8* kbuf = (u8*)kmalloc((size_t)len);
    if (!kbuf) {
        return (u64)-1;
    }
    copy_from_user(kbuf, buf, (size_t)len);
    ssize_t n = udp_send(sock, kbuf, (size_t)len, dst, port);
    kfree(kbuf);
    return n < 0 ? (u64)-1 : (u64)n;
}

u64 sys_socket_close(u64 fd) {
    fd_free(fd);
    return 0;
}
