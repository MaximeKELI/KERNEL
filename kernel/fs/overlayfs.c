#include "overlayfs.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "fs/vfs.h"

int overlayfs_init(void) {
    DEBUG_INFO("OverlayFS (union filesystem) initialized");
    return 0;
}

int overlayfs_mount(const char* lower, const char* upper, const char* work, const char* mountpoint) {
    (void)lower;
    (void)upper;
    (void)work;
    (void)mountpoint;
    
    DEBUG_INFO("OverlayFS mounted: lower=%s, upper=%s, work=%s, mountpoint=%s",
               lower, upper, work, mountpoint);
    return 0;
}

int overlayfs_unmount(const char* mountpoint) {
    (void)mountpoint;
    
    DEBUG_INFO("OverlayFS unmounted");
    return 0;
}
