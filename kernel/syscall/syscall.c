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
#include "landlock.h"
#include "audit.h"
#include "memory.h"
#include "signal.h"
#include "kspp.h"
#include "fb_console.h"

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
    (syscall_func_t)sys_sigreturn,
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

static char audit_buf[64];

u64 syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    if (syscall_num >= sizeof(syscall_table) / sizeof(syscall_table[0])) {
        DEBUG_ERROR("Invalid syscall number: %llu", (unsigned long long)syscall_num);
        return (u64)-1;
    }

    process_t* proc = process_current();
    if (proc && !seccomp_check_syscall(syscall_num)) {
        snprintf(audit_buf, sizeof(audit_buf), "deny syscall %llu", (unsigned long long)syscall_num);
        audit_log(AUDIT_SYSCALL, audit_buf);
        return (u64)-1;
    }

    if (!kspp_stack_check()) {
        audit_log(AUDIT_SYSCALL, "stack canary fault");
        return (u64)-1;
    }

    syscall_func_t func = syscall_table[syscall_num];
    if (!func) {
        return (u64)-1;
    }

    u64 ret = func(arg1, arg2, arg3, arg4, arg5);

    if (syscall_num == SYS_OPEN || syscall_num == SYS_EXEC) {
        snprintf(audit_buf, sizeof(audit_buf), "syscall %llu ret=%llu",
                 (unsigned long long)syscall_num, (unsigned long long)ret);
        audit_log(AUDIT_SYSCALL, audit_buf);
    }

    return ret;
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
        proc->state = PROCESS_ZOMBIE;
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
        fb_console_write(str, count);
        return count;
    }
    return (u64)vfs_write_fd((int)fd, buf, (size_t)count);
}

u64 sys_read(u64 fd, void* buf, u64 count) {
    VALIDATE_PTR_RET(buf, 0);
    if (fd < 3) {
        return 0;
    }
    return (u64)vfs_read_fd((int)fd, buf, (size_t)count);
}

u64 sys_open(const char* path, u64 flags) {
    char kpath[256];
    if (!path) {
        return (u64)-1;
    }
    if (copy_from_user(kpath, path, sizeof(kpath) - 1) < 0) {
        return (u64)-1;
    }
    kpath[sizeof(kpath) - 1] = '\0';

    process_t* proc = process_current();
    u64 pid = proc ? proc->pid : 0;
    if (!landlock_check_access(pid, kpath, LANDLOCK_ACCESS_FS_READ)) {
        audit_log(AUDIT_PATH, "landlock deny open");
        return (u64)-1;
    }

    int fd = vfs_open_fd(kpath, flags);
    return fd < 0 ? (u64)-1 : (u64)fd;
}

u64 sys_close(u64 fd) {
    if (fd >= 3 && fd < 256) {
        return vfs_close_fd((int)fd) == 0 ? 0 : (u64)-1;
    }
    if (fd >= 3) {
        return sys_socket_close(fd);
    }
    return 0;
}

u64 sys_fork(void) {
    process_t* parent = process_current();
    if (!parent) {
        return (u64)-1;
    }
    u64 parent_pid = parent->pid;
    process_t* child = fork_process();
    if (!child) {
        return (u64)-1;
    }
    process_t* cur = process_current();
    if (cur && cur->pid != parent_pid && cur->parent_pid == parent_pid) {
        return 0;
    }
    return child->pid;
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

    seccomp_set_mode_strict();

    if (exec_run_path(kpath) < 0) {
        return (u64)-1;
    }
    return 0;
}

u64 sys_wait(u64 pid) {
    int status = 0;
    int r = wait_process(pid, &status);
    return r < 0 ? (u64)-1 : (u64)r;
}

#define USER_MMAP_BASE 0x500000UL
#define USER_MMAP_MAX  (USER_STACK_TOP - USER_MMAP_BASE)

u64 sys_mmap(void* addr, u64 length, u64 prot, u64 flags) {
    (void)addr;
    (void)prot;
    (void)flags;
    if (length == 0 || length > USER_MMAP_MAX) {
        return (u64)-1;
    }
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    void* virt = vmm_alloc_pages(pages);
    if (!virt) {
        return (u64)-1;
    }
    u64 va = (u64)virt;
    if (va < USER_MMAP_BASE) {
        va = USER_MMAP_BASE;
    }
    return va;
}

u64 sys_munmap(void* addr, u64 length) {
    if (!addr || length == 0) {
        return (u64)-1;
    }
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    vmm_free_pages(addr, pages);
    return 0;
}

u64 sys_sigreturn(void) {
    signal_return_from_handler();
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
