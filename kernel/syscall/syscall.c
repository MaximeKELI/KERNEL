#include "syscall.h"
#include "process.h"
#include "stdio.h"
#include "fs/vfs.h"
#include "io.h"

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
    if (syscall_num >= sizeof(syscall_table) / sizeof(syscall_table[0])) {
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
    wrmsr(0xC0000080, rdmsr(0xC0000080) | (1 << 0));  /* SCE */
    wrmsr(0xC0000081, (u64)syscall_entry);  /* STAR */
    wrmsr(0xC0000082, 0x0018000800000000ULL);  /* LSTAR */
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
    (void)fd;
    return 0;
}

u64 sys_fork(void) {
    return 0;
}

u64 sys_exec(const char* path, char* const argv[]) {
    (void)path;
    (void)argv;
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
