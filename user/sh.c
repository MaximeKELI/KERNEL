/* Minimal POSIX shell — musl/Linux ABI syscalls */
#define SYS_WRITE 1
#define SYS_READ  2
#define SYS_EXIT  0
#define SYS_EXEC  6

static long syscall6(long n, long a, long b, long c, long d, long e, long f) {
    long ret;
    __asm__ volatile(
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a), "S"(b), "d"(c), "r10"(d), "r8"(e), "r9"(f)
        : "rcx", "r11", "memory");
    (void)e;
    (void)f;
    return ret;
}

static void print(const char* s) {
    long len = 0;
    while (s[len]) {
        len++;
    }
    syscall6(SYS_WRITE, 1, (long)s, len, 0, 0, 0);
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
    long n = syscall6(SYS_READ, 0, (long)buf, 63, 0, 0, 0);
    if (n > 0) {
        buf[n] = '\0';
        if (streq(buf, "nettest\n") || streq(buf, "nettest")) {
            syscall6(SYS_EXEC, (long)"/nettest", 0, 0, 0, 0, 0);
        }
    }
    syscall6(SYS_EXIT, 0, 0, 0, 0, 0, 0);
    for (;;) {
        __asm__ volatile("hlt");
    }
}
