#include "devtmpfs.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "device.h"

#define MAX_DEV_NODES 512

typedef struct dev_node {
    char path[256];
    u32 major;
    u32 minor;
    u32 mode;
    bool exists;
} dev_node_t;

static dev_node_t dev_nodes[MAX_DEV_NODES];
static u32 next_node = 0;

int devtmpfs_init(void) {
    memset(dev_nodes, 0, sizeof(dev_nodes));
    
    /* Create standard device nodes */
    devtmpfs_create_device("/dev/null", 1, 3, 0666);
    devtmpfs_create_device("/dev/zero", 1, 5, 0666);
    devtmpfs_create_device("/dev/random", 1, 8, 0666);
    devtmpfs_create_device("/dev/urandom", 1, 9, 0666);
    
    DEBUG_INFO("Devtmpfs initialized");
    return 0;
}

int devtmpfs_mount(const char* mountpoint) {
    (void)mountpoint;
    return 0;
}

int devtmpfs_create_device(const char* path, u32 major, u32 minor, u32 mode) {
    if (next_node >= MAX_DEV_NODES) {
        DEBUG_ERROR("Maximum device nodes reached");
        return -1;
    }
    
    dev_node_t* node = &dev_nodes[next_node++];
    strncpy(node->path, path, sizeof(node->path) - 1);
    node->major = major;
    node->minor = minor;
    node->mode = mode;
    node->exists = true;
    
    DEBUG_INFO("Device node created: %s (%u,%u)", path, major, minor);
    return 0;
}

int devtmpfs_remove_device(const char* path) {
    for (u32 i = 0; i < next_node; i++) {
        if (strcmp(dev_nodes[i].path, path) == 0) {
            dev_nodes[i].exists = false;
            DEBUG_INFO("Device node removed: %s", path);
            return 0;
        }
    }
    return -1;
}

int devtmpfs_hotplug_add(const char* path, u32 major, u32 minor) {
    return devtmpfs_create_device(path, major, minor, 0666);
}

int devtmpfs_hotplug_remove(const char* path) {
    return devtmpfs_remove_device(path);
}
