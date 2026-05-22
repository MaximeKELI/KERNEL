#include "syscall.h"
#include "process.h"
#include "stdio.h"
#include "fs/vfs.h"
#include "io.h"
#include "validate.h"
#include "seccomp.h"
#include "dns.h"
#include "net_addr.h"
#include "uaccess.h"
#include "exec.h"
#include "ai_manager.h"

typedef u64 (*syscall_func_t)(u64, u64, u64, u64, u64);

static syscall_func_t syscall_table[] = {
    (syscall_func_t)sys_exit,
    (syscall_func_t)sys_write,
    (syscall_func_t)sys_read,
    (syscall_func_t)sys_open,
    (syscall_func_t)sys_close,
    (syscall_func_t)sys_fork,
    (syscall_func_t)sys_exec,
    (syscall_func_t)sys_wait,
    (syscall_func_t)sys_mmap,
    (syscall_func_t)sys_munmap,
    NULL,
    (syscall_func_t)sys_socket,
    (syscall_func_t)sys_bind,
    (syscall_func_t)sys_connect,
    (syscall_func_t)sys_listen,
    (syscall_func_t)sys_accept,
    (syscall_func_t)sys_send,
    (syscall_func_t)sys_recv,
    (syscall_func_t)sys_sendto,
    (syscall_func_t)sys_socket_close,
    (syscall_func_t)sys_dns_resolve,
    (syscall_func_t)sys_ai_metrics,
};

u64 syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    if (syscall_num >= sizeof(syscall_table) / sizeof(syscall_table[0])) {
        DEBUG_ERROR("Invalid syscall number: %llu", (unsigned long long)syscall_num);
        return (u64)-1;
    }

    process_t* proc = process_current();
    if (proc && !seccomp_check_syscall(syscall_num)) {
        DEBUG_ERROR("Syscall blocked by seccomp: %llu", (unsigned long long)syscall_num);
        return (u64)-1;
    }

    syscall_func_t func = syscall_table[syscall_num];
    if (!func) {
        return (u64)-1;
    }
    return func(arg1, arg2, arg3, arg4, arg5);
}

void syscall_init(void) {
    extern void syscall_entry(void);
    syscall_socket_init();
    u64 efer = rdmsr(0xC0000080);
    wrmsr(0xC0000080, efer | (1 << 0));
    wrmsr(0xC0000081, 0x0018000800000000ULL);
    wrmsr(0xC0000082, (u64)syscall_entry);
    wrmsr(0xC0000084, 0x200);
    printk("Syscall: ready\n");
}

u64 sys_exit(u64 status) {
    (void)status;
    process_t* proc = process_current();
    if (proc) {
        proc->state = PROCESS_DEAD;
        process_destroy(proc);
    }
    schedule();
    return 0;
}

u64 sys_write(u64 fd, const void* buf, u64 count) {
    VALIDATE_RANGE(fd, 0, 255);
    VALIDATE_PTR_RET(buf, 0);
    VALIDATE_RANGE(count, 0, 1024 * 1024);

    if (fd == 1 || fd == 2) {
        const char* str = (const char*)buf;
        for (u64 i = 0; i < count; i++) {
            printk("%c", str[i]);
        }
        return count;
    }
    return 0;
}

u64 sys_read(u64 fd, void* buf, u64 count) {
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}

u64 sys_open(const char* path, u64 flags) {
    (void)path;
    (void)flags;
    return 0;
}

u64 sys_close(u64 fd) {
    if (fd >= 3) {
        return sys_socket_close(fd);
    }
    return 0;
}

u64 sys_fork(void) {
    return 0;
}

u64 sys_exec(const char* path, char* const argv[]) {
    (void)argv;
    char kpath[256];
    if (!path) {
        return (u64)-1;
    }
    if (copy_from_user(kpath, path, sizeof(kpath) - 1) < 0) {
        return (u64)-1;
    }
    kpath[sizeof(kpath) - 1] = '\0';
    if (exec_run_path(kpath) < 0) {
        return (u64)-1;
    }
    return 0;
}

u64 sys_wait(u64 pid) {
    (void)pid;
    return 0;
}

u64 sys_mmap(void* addr, u64 length, u64 prot, u64 flags) {
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    return 0;
}

u64 sys_munmap(void* addr, u64 length) {
    (void)addr;
    (void)length;
    return 0;
}

u64 sys_ai_metrics(void* out_info, u64 size) {
    if (!out_info || size < sizeof(ai_user_info_t)) {
        return (u64)-1;
    }
    if (!ai_initialized) {
        return (u64)-1;
    }
    ai_user_info_t info;
    ai_fill_user_info(&info);
    if (copy_to_user(out_info, &info, sizeof(info)) < 0) {
        return (u64)-1;
    }
    return 0;
}

u64 sys_dns_resolve(const char* hostname, void* out_ip, u64 out_len) {
    if (!hostname || !out_ip || out_len < 4) {
        return (u64)-1;
    }
    ip_addr_t ip;
    if (dns_resolve_a(hostname, &ip) < 0) {
        return (u64)-1;
    }
    if (copy_to_user(out_ip, ip.addr, 4) < 0) {
        return (u64)-1;
    }
    return 0;
}
