/* Minimal userland network test (linked as static ELF, loaded by kernel later) */
typedef unsigned long u64;
typedef long i64;

#define SYS_WRITE  1
#define SYS_SOCKET 11
#define SYS_SENDTO 18
#define SYS_DNS        20
#define SYS_AI_METRICS 21

#define AI_USER_MAGIC 0x41494D31U

#define AF_INET 2
#define SOCK_DGRAM 2

struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

struct ai_user_info {
    unsigned int magic;
    unsigned int version;
    unsigned int cpu_usage;
    unsigned int memory_usage;
    unsigned int cpu_predict;
    unsigned int mem_predict;
    unsigned int health_score;
    unsigned int policy_mode;
    unsigned int goal_mode;
    unsigned int process_count;
    unsigned int decisions_total;
    unsigned int io_class_count;
    unsigned int net_class_count;
    unsigned int reserved[2];
};

static inline u64 syscall1(u64 n, u64 a1) {
    u64 ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static inline u64 syscall3(u64 n, u64 a1, u64 a2, u64 a3) {
    u64 ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                     : "rcx", "r11", "memory");
    return ret;
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

    struct ai_user_info ai;
    i64 ar = (i64)syscall3(SYS_AI_METRICS, (u64)&ai, (u64)sizeof(ai), 0);
    if (ar == 0 && ai.magic == AI_USER_MAGIC) {
        puts("AI health=");
        char hbuf[8];
        unsigned h = ai.health_score;
        hbuf[0] = '0' + (h / 100) % 10;
        hbuf[1] = '0' + (h / 10) % 10;
        hbuf[2] = '0' + h % 10;
        hbuf[3] = '/';
        hbuf[4] = '1';
        hbuf[5] = '0';
        hbuf[6] = '0';
        hbuf[7] = '\n';
        syscall3(SYS_WRITE, 1, (u64)hbuf, 8);
    }

    for (;;) {
        __asm__ volatile("hlt");
    }
}
