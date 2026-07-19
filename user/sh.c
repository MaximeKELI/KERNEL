/* Minimal interactive shell — Linux ABI syscalls, runs in ring 3 */
#define SYS_WRITE  1
#define SYS_READ   2
#define SYS_EXIT   0
#define SYS_EXEC   6
#define SYS_GETPID 26

static long syscall3(long n, long a, long b, long c) {
    long ret;
    __asm__ volatile(
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a), "S"(b), "d"(c)
        : "rcx", "r11", "memory");
    return ret;
}

static unsigned slen(const char* s) {
    unsigned n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static void print(const char* s) {
    syscall3(SYS_WRITE, 1, (long)s, slen(s));
}

static int streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* true if s starts with prefix */
static int starts(const char* s, const char* prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static void print_dec(unsigned v) {
    char buf[12];
    int i = 0;
    if (v == 0) {
        print("0");
        return;
    }
    while (v > 0 && i < 10) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0) {
        char c = buf[--i];
        syscall3(SYS_WRITE, 1, (long)&c, 1);
    }
}

static void help(void) {
    print("commands: help, echo <text>, nettest, exit\n");
}

void _start(void) {
    print("sh: interactive shell (ring 3, Linux ABI)\n");
    long pid = syscall3(SYS_GETPID, 0, 0, 0);
    if (pid > 0) {
        print("pid=");
        print_dec((unsigned)pid);
        print("\n");
    }
    help();

    char buf[128];
    for (;;) {
        print("sh> ");
        long n = syscall3(SYS_READ, 0, (long)buf, sizeof(buf) - 1);
        if (n <= 0) {
            print("\n");
            break;                       /* EOF -> exit */
        }
        buf[n] = '\0';
        if (n > 0 && buf[n - 1] == '\n') {
            buf[n - 1] = '\0';           /* strip the newline */
        }

        if (buf[0] == '\0') {
            continue;                    /* empty line */
        }
        if (streq(buf, "exit") || streq(buf, "quit")) {
            break;
        }
        if (streq(buf, "help")) {
            help();
        } else if (streq(buf, "nettest")) {
            syscall3(SYS_EXEC, (long)"/nettest", 0, 0);
            print("exec /nettest failed\n");   /* only if exec returns */
        } else if (starts(buf, "echo ")) {
            print(buf + 5);
            print("\n");
        } else {
            print("unknown command: ");
            print(buf);
            print("\n");
        }
    }

    syscall3(SYS_EXIT, 0, 0, 0);
    for (;;) {
        __asm__ volatile("hlt");
    }
}
