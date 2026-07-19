#include "abi/linux_abi.h"
#include "abi/linux_syscall.h"
#include "abi/errno.h"
#include "syscall.h"
#include "process.h"
#include "scheduler.h"
#include "uaccess.h"
#include "string.h"
#include "stdio.h"
#include "io.h"
#include "fs/vfs.h"
#include "fs/ramfs.h"
#include "signal.h"
#include "drivers/timer.h"

#define IA32_FS_BASE 0xC0000100

/* Convert a legacy handler's (u64)-1 failure into a Linux -errno. */
static long map_ret(u64 r, long err_if_fail) {
    return (r == (u64)-1) ? err_if_fail : (long)r;
}

long lx_arch_prctl(u64 code, u64 addr) {
    process_t* p = process_current();
    switch (code) {
    case ARCH_SET_FS:
        if (p) p->fs_base = addr;
        wrmsr(IA32_FS_BASE, addr);
        return 0;
    case ARCH_GET_FS: {
        u64 v = p ? p->fs_base : rdmsr(IA32_FS_BASE);
        if (copy_to_user((void*)addr, &v, sizeof(v)) < 0) return -EFAULT;
        return 0;
    }
    default:
        return -EINVAL;
    }
}

long lx_set_tid_address(u64 tidptr) {
    process_t* p = process_current();
    if (p) p->clear_child_tid = tidptr;
    return p ? (long)p->pid : 0;
}

typedef struct {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
} utsname_t;

long lx_uname(void* buf) {
    if (!buf) return -EFAULT;
    utsname_t u;
    memset(&u, 0, sizeof(u));
    strcpy(u.sysname, "KERNEL");
    strcpy(u.nodename, "kernel");
    strcpy(u.release, "0.6.0-rattraper");
    strcpy(u.version, "#1 SMP KERNEL");
    strcpy(u.machine, "x86_64");
    if (copy_to_user(buf, &u, sizeof(u)) < 0) return -EFAULT;
    return 0;
}

long lx_getcwd(void* buf, u64 size) {
    if (!buf || size < 2) return -ERANGE;
    char cwd[2] = "/";
    if (copy_to_user(buf, cwd, 2) < 0) return -EFAULT;
    return 2;
}

typedef struct { u64 base; u64 len; } iovec_k_t;

static long lx_writev(u64 fd, u64 iov_ptr, u64 iovcnt) {
    if (iovcnt > 1024) return -EINVAL;
    long total = 0;
    for (u64 i = 0; i < iovcnt; i++) {
        iovec_k_t iov;
        if (copy_from_user(&iov, (void*)(iov_ptr + i * sizeof(iov)), sizeof(iov)) < 0) {
            return -EFAULT;
        }
        if (iov.len == 0) continue;
        u64 w = sys_write(fd, (const void*)iov.base, iov.len);
        if (w == (u64)-1) return total ? total : -EBADF;
        total += (long)w;
    }
    return total;
}

typedef struct { i64 tv_sec; i64 tv_nsec; } timespec_k_t;

static long lx_nanosleep(u64 req_ptr, u64 rem_ptr) {
    timespec_k_t req;
    if (copy_from_user(&req, (void*)req_ptr, sizeof(req)) < 0) return -EFAULT;
    u64 ms = (u64)req.tv_sec * 1000 + (u64)req.tv_nsec / 1000000;
    u64 start = timer_get_ticks();
    while (timer_get_ticks() - start < ms) {
        schedule();
    }
    if (rem_ptr) {
        timespec_k_t z = {0, 0};
        copy_to_user((void*)rem_ptr, &z, sizeof(z));
    }
    return 0;
}

static long lx_chdir(u64 path_ptr) {
    char kp[256];
    if (copy_from_user(kp, (void*)path_ptr, sizeof(kp) - 1) < 0) return -EFAULT;
    kp[sizeof(kp) - 1] = '\0';
    return ramfs_resolve(kp) ? 0 : -ENOENT;
}

long linux_syscall(long nr, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) {
    (void)a6;
    switch (nr) {
    case __NR_read:    return map_ret(sys_read(a1, (void*)a2, a3), -EBADF);
    case __NR_write:   return map_ret(sys_write(a1, (const void*)a2, a3), -EBADF);
    case __NR_open:    return map_ret(sys_open((const char*)a1, a2), -ENOENT);
    case __NR_close:   return map_ret(sys_close(a1), -EBADF);
    case __NR_lseek:   return map_ret(sys_lseek(a1, (i64)a2, a3), -ESPIPE);
    case __NR_mmap:    return (long)sys_mmap((void*)a1, a2, a3, a4);
    case __NR_mprotect:return map_ret(sys_mprotect((void*)a1, a2, a3), -EINVAL);
    case __NR_munmap:  return map_ret(sys_munmap((void*)a1, a2), -EINVAL);
    case __NR_brk:     return (long)sys_brk(a1);
    case __NR_rt_sigaction:  return map_ret(sys_rt_sigaction(a1, (void*)a2, (void*)a3), -EINVAL);
    case __NR_rt_sigprocmask:return map_ret(sys_rt_sigprocmask(a1, (void*)a2, (void*)a3), -EINVAL);
    case __NR_rt_sigreturn:  return (long)sys_rt_sigreturn();
    case __NR_ioctl:   return 0;   /* pretend success for tty ioctls */
    case __NR_writev:  return lx_writev(a1, a2, a3);
    case __NR_pipe:    return map_ret(sys_pipe((int*)a1), -EMFILE);
    case __NR_dup:     return map_ret(sys_dup(a1), -EBADF);
    case __NR_dup2:    return map_ret(sys_dup2(a1, a2), -EBADF);
    case __NR_nanosleep: return lx_nanosleep(a1, a2);
    case __NR_getpid:  return (long)sys_getpid();
    case __NR_gettid:  return (long)sys_getpid();
    case __NR_fork:    return (long)sys_fork();
    case __NR_execve:  return map_ret(sys_exec((const char*)a1, (char* const*)a2), -ENOENT);
    case __NR_exit:
    case __NR_exit_group: sys_exit(a1); return 0;
    case __NR_wait4:   return map_ret(sys_wait(a1, (int*)a2), -ECHILD);
    case __NR_kill:    return map_ret(sys_kill(a1, a2), -ESRCH);
    case __NR_uname:   return lx_uname((void*)a1);
    case __NR_getcwd:  return lx_getcwd((void*)a1, a2);
    case __NR_chdir:   return lx_chdir(a1);
    case __NR_mkdir:   return map_ret(sys_mkdir((const char*)a1, a2), -EEXIST);
    case __NR_rmdir:   return map_ret(sys_rmdir((const char*)a1), -ENOTEMPTY);
    case __NR_unlink:  return map_ret(sys_unlink((const char*)a1), -ENOENT);
    case __NR_ftruncate: return map_ret(sys_ftruncate(a1, a2), -EBADF);
    case __NR_getdents64: return map_ret(sys_getdents(a1, (void*)a2, a3), -EBADF);
    case __NR_arch_prctl: return lx_arch_prctl(a1, a2);
    case __NR_set_tid_address: return lx_set_tid_address(a1);
    case __NR_clock_gettime: return map_ret(sys_clock_gettime(a1, (void*)a2), -EINVAL);
    case __NR_poll:    return (long)sys_poll((void*)a1, a2, a3);
    case __NR_socket:  return (long)sys_socket(a1, a2, a3);
    case __NR_connect: return (long)sys_connect(a1, (void*)a2, a3);
    case __NR_bind:    return (long)sys_bind(a1, (void*)a2, a3);
    case __NR_listen:  return (long)sys_listen(a1, a2);
    case __NR_accept:  return (long)sys_accept(a1, (void*)a2, a3);
    case __NR_sendto:  return (long)sys_sendto(a1, (void*)a2, a3, a4, (void*)a5, a6);
    case __NR_recvfrom:return (long)sys_recv(a1, (void*)a2, a3, a4);
    default:
        return -ENOSYS;
    }
}
