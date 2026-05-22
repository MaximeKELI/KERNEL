/* Minimal POSIX shell — Linux ABI syscalls */
#define SYS_WRITE 1
#define SYS_READ  2
#define SYS_EXIT  0
#define SYS_EXEC  6

static long syscall3(long n, long a, long b, long c) {
    long ret;
    __asm__ volatile(
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a), "S"(b), "d"(c)
        : "rcx", "r11", "memory");
    return ret;
}

static void print(const char* s) {
    long len = 0;
    while (s[len]) {
        len++;
    }
    syscall3(SYS_WRITE, 1, (long)s, len);
}

static int streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

void _start(void) {
    print("sh: kernel shell (Linux ABI)\n");
    print("sh> ");
    char buf[64];
    long n = syscall3(SYS_READ, 0, (long)buf, 63);
    if (n > 0) {
        buf[n] = '\0';
        if (streq(buf, "nettest\n") || streq(buf, "nettest")) {
            syscall3(SYS_EXEC, (long)"/nettest", 0, 0);
        }
    }
    syscall3(SYS_EXIT, 0, 0, 0);
    for (;;) {
        __asm__ volatile("hlt");
    }
}
