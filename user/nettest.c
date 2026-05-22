/* Minimal userland network test (linked as static ELF, loaded by kernel later) */
typedef unsigned long u64;
typedef long i64;

#define SYS_WRITE  1
#define SYS_SOCKET 11
#define SYS_SENDTO 18
#define SYS_DNS    20

#define AF_INET 2
#define SOCK_DGRAM 2

struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

static inline u64 syscall6(u64 n, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) {
    u64 ret;
    __asm__ volatile(
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r10"(a4), "r8"(a5), "r9"(a6)
        : "rcx", "r11", "memory");
    return ret;
}

static inline u64 syscall1(u64 n, u64 a1) {
    return syscall6(n, a1, 0, 0, 0, 0, 0);
}

static inline u64 syscall3(u64 n, u64 a1, u64 a2, u64 a3) {
    return syscall6(n, a1, a2, a3, 0, 0, 0);
}

static void puts(const char* s) {
    u64 len = 0;
    while (s[len]) {
        len++;
    }
    syscall3(SYS_WRITE, 1, (u64)s, len);
}

void _start(void) {
    puts("nettest: userland syscalls OK\n");

    u64 fd = syscall3(SYS_SOCKET, AF_INET, SOCK_DGRAM, 0);
    if ((i64)fd < 0) {
        puts("socket failed\n");
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    unsigned char ip[4] = {0, 0, 0, 0};
    i64 dr = (i64)syscall3(SYS_DNS, (u64) "example.com", (u64)ip, 4);
    if (dr == 0) {
        puts("dns example.com -> ");
        for (int i = 0; i < 4; i++) {
            char buf[4];
            unsigned v = ip[i];
            buf[0] = '0' + (v / 100) % 10;
            buf[1] = '0' + (v / 10) % 10;
            buf[2] = '0' + v % 10;
            buf[3] = (i < 3) ? '.' : '\n';
            syscall3(SYS_WRITE, 1, (u64)buf, 4);
        }
    } else {
        puts("dns failed\n");
    }

    for (;;) {
        __asm__ volatile("hlt");
    }
}
