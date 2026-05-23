#include "fs/vfs.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "validate.h"
#include "tmpfs.h"
#include "ext2.h"
#include "epoll.h"

#define VFS_MAX_FD 256
#define VFS_MAX_BACKENDS 8

typedef enum {
    VFS_BACKEND_NONE = 0,
    VFS_BACKEND_TMPFS,
    VFS_BACKEND_EXT2,
} vfs_backend_type_t;

typedef struct vfs_fd_entry {
    vfs_file_t* file;
    vfs_ops_t* ops;
    vfs_backend_type_t backend;
    bool in_use;
} vfs_fd_entry_t;

static vfs_fd_entry_t fd_table[VFS_MAX_FD];
static u32 next_vfs_fd = 3;
static vfs_fs_ops_t* mounted_fs[VFS_MAX_BACKENDS];
static u32 mount_count = 0;
static bool ext2_mounted = false;

void vfs_init(void) {
    memset(fd_table, 0, sizeof(fd_table));
    next_vfs_fd = 3;
    mount_count = 0;
    ext2_mounted = false;
    printk("VFS: Initialized (tmpfs + ext2)\n");
}

static vfs_backend_type_t vfs_path_backend(const char* path) {
    if (!path) {
        return VFS_BACKEND_NONE;
    }
    if (strncmp(path, "/tmp/", 5) == 0 || strcmp(path, "/tmp") == 0) {
        return VFS_BACKEND_TMPFS;
    }
    if (strncmp(path, "/etc/", 5) == 0) {
        return VFS_BACKEND_TMPFS;
    }
    if (strncmp(path, "/boot/", 6) == 0 || strncmp(path, "/", 1) == 0) {
        if (ext2_mounted) {
            return VFS_BACKEND_EXT2;
        }
    }
    return VFS_BACKEND_TMPFS;
}

int vfs_mount(const char* source, const char* target, vfs_fs_ops_t* fs_ops) {
    VALIDATE_STRING(source, 4096);
    VALIDATE_STRING(target, 4096);
    VALIDATE_PTR(fs_ops);

    if (mount_count >= VFS_MAX_BACKENDS) {
        return -1;
    }
    mounted_fs[mount_count++] = fs_ops;
    if (fs_ops->mount) {
        int r = fs_ops->mount(source, target);
        if (strcmp(source, "hda") == 0 || strstr(source, "ext2")) {
            ext2_mounted = (r == 0);
        }
        return r;
    }
    return 0;
}

int vfs_unmount(const char* target) {
    VALIDATE_STRING(target, 4096);
    (void)target;
    return 0;
}

static int vfs_alloc_fd(vfs_file_t* file, vfs_ops_t* ops, vfs_backend_type_t backend) {
    for (u32 fd = next_vfs_fd; fd < VFS_MAX_FD; fd++) {
        if (!fd_table[fd].in_use) {
            fd_table[fd].file = file;
            fd_table[fd].ops = ops;
            fd_table[fd].backend = backend;
            fd_table[fd].in_use = true;
            if (fd >= next_vfs_fd) {
                next_vfs_fd = fd + 1;
            }
            return (int)fd;
        }
    }
    return -1;
}

vfs_file_t* vfs_open(const char* path, u64 flags) {
    VALIDATE_STRING(path, 4096);
    VALIDATE_FLAGS(flags, 0xFFFFFFFF);

    vfs_backend_type_t be = vfs_path_backend(path);
    vfs_ops_t* ops = NULL;

    if (be == VFS_BACKEND_TMPFS) {
        ops = tmpfs_get_file_ops();
    } else if (be == VFS_BACKEND_EXT2) {
        ops = ext2_get_file_ops();
    }

    if (!ops || !ops->open) {
        return NULL;
    }

    vfs_file_t* file = (vfs_file_t*)kmalloc(sizeof(vfs_file_t));
    if (!file) {
        return NULL;
    }
    memset(file, 0, sizeof(*file));
    file->flags = flags;
    file->type = VFS_FILE;

    if (ops->open(file, path) < 0) {
        kfree(file);
        return NULL;
    }
    file->private_data = (void*)(uintptr_t)be;
    return file;
}

int vfs_open_fd(const char* path, u64 flags) {
    vfs_file_t* f = vfs_open(path, flags);
    if (!f) {
        return -1;
    }
    vfs_backend_type_t be = (vfs_backend_type_t)(uintptr_t)f->private_data;
    vfs_ops_t* ops = (be == VFS_BACKEND_EXT2) ? ext2_get_file_ops() : tmpfs_get_file_ops();
    return vfs_alloc_fd(f, ops, be);
}

int vfs_close_fd(int fd) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD || !fd_table[fd].in_use) {
        return -1;
    }
    vfs_fd_entry_t* e = &fd_table[fd];
    if (e->ops && e->ops->close && e->file) {
        e->ops->close(e->file);
    }
    kfree(e->file);
    memset(e, 0, sizeof(*e));
    return 0;
}

ssize_t vfs_read_fd(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD || !fd_table[fd].in_use) {
        return -1;
    }
    vfs_fd_entry_t* e = &fd_table[fd];
    if (!e->ops || !e->ops->read || !e->file) {
        return -1;
    }
    return e->ops->read(e->file, buf, count);
}

ssize_t vfs_write_fd(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD || !fd_table[fd].in_use) {
        return -1;
    }
    vfs_fd_entry_t* e = &fd_table[fd];
    if (!e->ops || !e->ops->write || !e->file) {
        return -1;
    }
    return e->ops->write(e->file, buf, count);
}

int vfs_close(vfs_file_t* file) {
    VALIDATE_PTR(file);
    kfree(file);
    return 0;
}

ssize_t vfs_read(vfs_file_t* file, void* buf, size_t count) {
    VALIDATE_PTR_RET(file, -1);
    VALIDATE_PTR_RET(buf, -1);
    vfs_backend_type_t be = (vfs_backend_type_t)(uintptr_t)file->private_data;
    vfs_ops_t* ops = (be == VFS_BACKEND_EXT2) ? ext2_get_file_ops() : tmpfs_get_file_ops();
    if (ops && ops->read) {
        return ops->read(file, buf, count);
    }
    return -1;
}

ssize_t vfs_write(vfs_file_t* file, const void* buf, size_t count) {
    VALIDATE_PTR_RET(file, -1);
    VALIDATE_PTR_RET(buf, -1);
    vfs_backend_type_t be = (vfs_backend_type_t)(uintptr_t)file->private_data;
    vfs_ops_t* ops = (be == VFS_BACKEND_EXT2) ? ext2_get_file_ops() : tmpfs_get_file_ops();
    if (ops && ops->write) {
        return ops->write(file, buf, count);
    }
    return -1;
}

ssize_t vfs_read_path(const char* path, void* buf, size_t count) {
    int fd = vfs_open_fd(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    ssize_t n = vfs_read_fd(fd, buf, count);
    vfs_close_fd(fd);
    return n;
}

ssize_t vfs_write_path(const char* path, const void* buf, size_t count) {
    int fd = vfs_open_fd(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        return -1;
    }
    ssize_t n = vfs_write_fd(fd, buf, count);
    vfs_close_fd(fd);
    return n;
}

void vfs_register_filesystem(const char* name, vfs_fs_ops_t* fs_ops) {
    VALIDATE_STRING(name, 256);
    VALIDATE_PTR_VOID(fs_ops);
    vfs_mount(name, "/", fs_ops);
}

int vfs_fd_poll_events(int fd) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD || !fd_table[fd].in_use) {
        return 0;
    }
    vfs_file_t* f = fd_table[fd].file;
    if (!f) {
        return 0;
    }
    if (f->offset < f->size || f->size > 0) {
        return EPOLLIN;
    }
    return 0;
}
