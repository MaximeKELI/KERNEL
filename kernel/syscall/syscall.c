#include "syscall.h"
#include "process.h"
#include "stdio.h"
#include "fs/vfs.h"
#include "io.h"
#include "validate.h"
#include "seccomp.h"

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
    (syscall_func_t)sys_munmap
};

void syscall_handler(u64 syscall_num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    /* Validate syscall number */
    if (syscall_num >= sizeof(syscall_table) / sizeof(syscall_table[0])) {
        DEBUG_ERROR("Invalid syscall number: %llu", (unsigned long long)syscall_num);
        return;
    }
    
    /* Check seccomp filter */
    process_t* proc = process_current();
    if (proc && !seccomp_check_syscall(syscall_num)) {
        DEBUG_ERROR("Syscall blocked by seccomp: %llu", (unsigned long long)syscall_num);
        return;
    }
    
    syscall_func_t func = syscall_table[syscall_num];
    if (func) {
        func(arg1, arg2, arg3, arg4, arg5);
    }
}

void syscall_init(void) {
    /* Setup syscall MSRs */
    extern void syscall_entry(void);
    u64 efer = rdmsr(0xC0000080);
    wrmsr(0xC0000080, efer | (1 << 0));  /* SCE */
    wrmsr(0xC0000081, 0x0018000800000000ULL);  /* STAR */
    wrmsr(0xC0000082, (u64)syscall_entry);  /* LSTAR */
    wrmsr(0xC0000084, 0x200);  /* SFMASK */
    
    printk("Syscall: Initialized\n");
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
    /* Validate parameters */
    VALIDATE_RANGE(fd, 0, 255);
    VALIDATE_PTR_RET(buf, 0);
    VALIDATE_RANGE(count, 0, 1024 * 1024); /* Max 1MB per write */
    
    if (fd == 1 || fd == 2) {  /* stdout/stderr */
        const char* str = (const char*)buf;
        for (u64 i = 0; i < count; i++) {
            printk("%c", str[i]);
        }
        return count;
    }
    return 0;
}

u64 sys_read(u64 fd, void* buf, u64 count) {
    /* Validate parameters */
    VALIDATE_RANGE(fd, 0, 255);
    VALIDATE_PTR_RET(buf, 0);
    VALIDATE_RANGE(count, 0, 1024 * 1024); /* Max 1MB per read */
    
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}

u64 sys_open(const char* path, u64 flags) {
    /* Validate parameters */
    VALIDATE_STRING(path, 4096); /* Max path length */
    VALIDATE_FLAGS(flags, 0xFFFFFFFF); /* Validate flags */
    
    (void)path;
    (void)flags;
    return 0;
}

u64 sys_close(u64 fd) {
    (void)fd;
    return 0;
}

u64 sys_fork(void) {
    return 0;
}

u64 sys_exec(const char* path, char* const argv[]) {
    /* Validate parameters */
    VALIDATE_STRING(path, 4096); /* Max path length */
    /* argv can be NULL, but if not NULL, validate it */
    if (argv) {
        /* Validate argv array (check first few entries) */
        for (int i = 0; i < 64 && argv[i]; i++) {
            VALIDATE_STRING(argv[i], 4096);
        }
    }
    
    (void)path;
    (void)argv;
    return 0;
}

u64 sys_wait(u64 pid) {
    (void)pid;
    return 0;
}

u64 sys_mmap(void* addr, u64 length, u64 prot, u64 flags) {
    /* Validate parameters */
    VALIDATE_RANGE(length, 1, 1024 * 1024 * 1024); /* Max 1GB */
    VALIDATE_FLAGS(prot, 0x7); /* PROT_READ, PROT_WRITE, PROT_EXEC */
    VALIDATE_FLAGS(flags, 0xFFFFFFFF); /* Validate flags */
    
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    return 0;
}

u64 sys_munmap(void* addr, u64 length) {
    /* Validate parameters */
    VALIDATE_PTR_RET(addr, 0);
    VALIDATE_RANGE(length, 1, 1024 * 1024 * 1024); /* Max 1GB */
    
    (void)addr;
    (void)length;
    return 0;
}
