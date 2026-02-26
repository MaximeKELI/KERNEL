#include "tmpfs.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "fs/vfs.h"

static void* tmpfs_root = NULL;
static size_t tmpfs_size = 0;

static int tmpfs_mount_fs(const char* source, const char* target) {
    (void)source;
    (void)target;
    
    tmpfs_size = 10 * 1024 * 1024; /* 10MB */
    tmpfs_root = vmm_alloc_pages((tmpfs_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!tmpfs_root) {
        DEBUG_ERROR("Failed to allocate tmpfs");
        return -1;
    }
    
    DEBUG_INFO("Tmpfs mounted: size=%u KB", (u32)(tmpfs_size / 1024));
    return 0;
}

static int tmpfs_unmount_fs(const char* target) {
    (void)target;
    
    if (tmpfs_root) {
        vmm_free_pages(tmpfs_root, (tmpfs_size + PAGE_SIZE - 1) / PAGE_SIZE);
        tmpfs_root = NULL;
    }
    
    return 0;
}

static vfs_fs_ops_t tmpfs_fs_ops = {
    .mount = tmpfs_mount_fs,
    .unmount = tmpfs_unmount_fs,
    .get_ops = NULL
};

int tmpfs_init(void) {
    DEBUG_INFO("Tmpfs (temporary filesystem) initialized");
    return 0;
}

int tmpfs_mount(const char* mountpoint, size_t size) {
    tmpfs_size = size;
    return vfs_mount("tmpfs", mountpoint, &tmpfs_fs_ops);
}

vfs_fs_ops_t* tmpfs_get_fs_ops(void) {
    return &tmpfs_fs_ops;
}
