/*
 * Example System Call Implementation
 * 
 * This shows how to add a new system call to the kernel.
 */

#include "syscall.h"
#include "stdio.h"
#include "debug.h"
#include "validate.h"
#include "process.h"

/* Example system call: get_kernel_version
 * Returns kernel version string
 */
static long sys_get_kernel_version(char* buf, size_t len) {
    VALIDATE_PTR_RET(buf, -1);
    VALIDATE_RANGE(len, 0, 256);
    
    const char* version = "Kernel v1.0.0";
    size_t version_len = strlen(version);
    
    if (len < version_len + 1) {
        return -1; /* Buffer too small */
    }
    
    /* Copy to user space (would use copy_to_user in real implementation) */
    memcpy(buf, version, version_len + 1);
    
    return (long)version_len;
}

/* Example system call: kernel_info
 * Returns kernel information structure
 */
typedef struct kernel_info {
    u32 version_major;
    u32 version_minor;
    u32 version_patch;
    u64 total_memory;
    u32 cpu_count;
} kernel_info_t;

static long sys_kernel_info(kernel_info_t* info) {
    VALIDATE_PTR_RET(info, -1);
    
    /* Fill kernel info */
    info->version_major = 1;
    info->version_minor = 0;
    info->version_patch = 0;
    
    extern size_t pmm_get_total_pages(void);
    info->total_memory = pmm_get_total_pages() * 4096;
    
    extern u32 smp_get_cpu_count(void);
    info->cpu_count = smp_get_cpu_count();
    
    return 0;
}

/* Register system calls */
void example_syscalls_register(void) {
    /* Would register in syscall table */
    /* syscall_register(SYS_GET_KERNEL_VERSION, sys_get_kernel_version); */
    /* syscall_register(SYS_KERNEL_INFO, sys_kernel_info); */
    
    DEBUG_INFO("Example system calls registered");
}
