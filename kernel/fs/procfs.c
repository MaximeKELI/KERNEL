#include "procfs.h"
#include "kernel.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "process.h"
#include "scheduler.h"

#define MAX_PROC_ENTRIES 256

static proc_entry_t proc_entries[MAX_PROC_ENTRIES];
static proc_entry_t* proc_root = NULL;
static u32 next_entry = 0;

static ssize_t proc_read_cpuinfo(void* buf, size_t count) {
    char info[512];
    int len = snprintf(info, sizeof(info),
        "processor\t: 0\n"
        "vendor_id\t: GenuineIntel\n"
        "cpu family\t: 6\n"
        "model\t\t: 0\n"
        "model name\t: x86_64 CPU\n"
        "stepping\t: 0\n"
        "cpu MHz\t\t: 2400.000\n"
        "cache size\t: 8192 KB\n"
        "bogomips\t: 4800.00\n");
    
    size_t to_copy = (len < (int)count) ? len : count;
    memcpy(buf, info, to_copy);
    return to_copy;
}

static ssize_t proc_read_meminfo(void* buf, size_t count) {
    char info[512];
    extern size_t pmm_get_total_pages(void);
    extern size_t pmm_get_free_pages(void);
    
    size_t total = pmm_get_total_pages() * PAGE_SIZE;
    size_t free = pmm_get_free_pages() * PAGE_SIZE;
    size_t used = total - free;
    
    int len = snprintf(info, sizeof(info),
        "MemTotal:     %u kB\n"
        "MemFree:      %u kB\n"
        "MemUsed:      %u kB\n"
        "Buffers:      0 kB\n"
        "Cached:       0 kB\n",
        (u32)(total / 1024),
        (u32)(free / 1024),
        (u32)(used / 1024));
    
    size_t to_copy = (len < (int)count) ? len : count;
    memcpy(buf, info, to_copy);
    return to_copy;
}

static ssize_t proc_read_version(void* buf, size_t count) {
    char info[256];
    int len = snprintf(info, sizeof(info),
        "Kernel v%d.%d.%d\n"
        "Compiled: %s %s\n",
        KERNEL_VERSION_MAJOR,
        KERNEL_VERSION_MINOR,
        KERNEL_VERSION_PATCH,
        __DATE__, __TIME__);
    
    size_t to_copy = (len < (int)count) ? len : count;
    memcpy(buf, info, to_copy);
    return to_copy;
}

int procfs_init(void) {
    memset(proc_entries, 0, sizeof(proc_entries));
    
    /* Create root */
    proc_root = &proc_entries[next_entry++];
    strncpy(proc_root->name, "proc", sizeof(proc_root->name) - 1);
    proc_root->name[sizeof(proc_root->name) - 1] = '\0';
    
    /* Create standard entries */
    proc_entry_t* cpuinfo = &proc_entries[next_entry++];
    strncpy(cpuinfo->name, "cpuinfo", sizeof(cpuinfo->name) - 1);
    cpuinfo->name[sizeof(cpuinfo->name) - 1] = '\0';
    cpuinfo->parent = proc_root;
    
    proc_entry_t* meminfo = &proc_entries[next_entry++];
    strncpy(meminfo->name, "meminfo", sizeof(meminfo->name) - 1);
    meminfo->name[sizeof(meminfo->name) - 1] = '\0';
    meminfo->parent = proc_root;
    
    proc_entry_t* version = &proc_entries[next_entry++];
    strncpy(version->name, "version", sizeof(version->name) - 1);
    version->name[sizeof(version->name) - 1] = '\0';
    version->parent = proc_root;
    
    DEBUG_INFO("Proc filesystem initialized");
    return 0;
}

int procfs_mount(const char* mountpoint) {
    (void)mountpoint;
    return 0;
}

int proc_create(const char* path, vfs_file_t* file) {
    (void)path;
    (void)file;
    return 0;
}

int proc_remove(const char* path) {
    (void)path;
    return 0;
}

ssize_t proc_read(const char* path, void* buf, size_t count) {
    if (strcmp(path, "/proc/cpuinfo") == 0) {
        return proc_read_cpuinfo(buf, count);
    } else if (strcmp(path, "/proc/meminfo") == 0) {
        return proc_read_meminfo(buf, count);
    } else if (strcmp(path, "/proc/version") == 0) {
        return proc_read_version(buf, count);
    }
    return 0;
}
